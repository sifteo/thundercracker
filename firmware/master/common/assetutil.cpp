/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "assetutil.h"
#include "assetslot.h"
#include "cubeslots.h"
#include "cube.h"
#include "svmruntime.h"
#include "svmmemory.h"
#include "svmloader.h"


_SYSAssetGroupCube *AssetUtil::mapGroupCube(SvmMemory::VirtAddr group, _SYSCubeID cid)
{
	ASSERT(cid < _SYS_NUM_CUBE_SLOTS);

	SvmMemory::VirtAddr va = group + sizeof(_SYSAssetGroup) + cid * sizeof(_SYSAssetGroupCube);
	SvmMemory::PhysAddr pa;

	if (!SvmMemory::mapRAM(va, sizeof(_SYSAssetGroupCube), pa))
		return 0;

	return reinterpret_cast<_SYSAssetGroupCube*>(pa);
}

_SYSAssetLoaderCube *AssetUtil::mapLoaderCube(SvmMemory::VirtAddr loader, _SYSCubeID cid)
{
	ASSERT(cid < _SYS_NUM_CUBE_SLOTS);

	SvmMemory::VirtAddr va = loader + sizeof(_SYSAssetLoader) + cid * sizeof(_SYSAssetLoaderCube);
	SvmMemory::PhysAddr pa;

	if (!SvmMemory::mapRAM(va, sizeof(_SYSAssetLoaderCube), pa))
		return 0;

	return reinterpret_cast<_SYSAssetLoaderCube*>(pa);
}

unsigned AssetUtil::loadedBaseAddr(SvmMemory::VirtAddr group, _SYSCubeID cid)
{
	_SYSAssetGroupCube *gc = mapGroupCube(group, cid);
	return gc ? gc->baseAddr : 0;
}

bool AssetUtil::isValidConfig(const _SYSAssetConfiguration *cfg, unsigned cfgSize)
{
	/*
	 * Initial validation for a _SYSAssetConfiguration. We can't rely exclusively
	 * on this pre-validation, since we're dealing with data in user RAM that
	 * may change at any time, but we can use this validation to catch unintentional
	 * errors early and deliver an appropriate fault.
	 */

	// Too large to possibly work?
	if (cfgSize > _SYS_ASSET_GROUPS_PER_SLOT * _SYS_ASSET_SLOTS_PER_BANK)
		return false;

	struct {
		uint16_t slotTiles[_SYS_ASSET_SLOTS_PER_BANK];
		uint8_t slotGroups[_SYS_ASSET_SLOTS_PER_BANK];
	} count;
	memset(&count, 0, sizeof count);

	while (cfgSize) {
		unsigned numTiles = roundup<_SYS_ASSET_GROUP_SIZE_UNIT>(cfg->numTiles);
		unsigned slot = cfg->slot;
		SvmMemory::VirtAddr groupVA = cfg->pGroup;
		SvmMemory::PhysAddr groupPA;
		_SYSVolumeHandle volHandle = cfg->volume;

		if (volHandle) {
			// Make sure the handle is signed and this is a real volume
			FlashVolume volume(volHandle);
			if (!volume.isValid()) {
				LOG(("ASSET: Bad volume handle 0x%08x in _SYSAssetConfiguration\n", volHandle));
				return false;
			}
		}

		if (!SvmMemory::mapRAM(groupVA, sizeof(_SYSAssetGroup), groupPA)) {
			LOG(("ASSET: Bad _SYSAssetGroup pointer 0x%08x in _SYSAssetConfiguration\n", unsigned(groupVA)));
			return false;
		}

		if (slot >= _SYS_ASSET_SLOTS_PER_BANK || !VirtAssetSlots::isSlotBound(slot)) {
			LOG(("ASSET: Bad slot number %d in _SYSAssetConfiguration\n", slot));
	    	return false;
	    }

		if (count.slotGroups[slot] >= _SYS_ASSET_GROUPS_PER_SLOT) {
			LOG(("ASSET: Bad _SYSAssetConfiguration, too many groups in slot %d\n", slot));
			return false;
		}

		if (unsigned(count.slotTiles[slot]) + numTiles >= _SYS_TILES_PER_ASSETSLOT) {
			LOG(("ASSET: Bad _SYSAssetConfiguration, too many tiles in slot %d\n", slot));
			return false;
		}

		count.slotGroups[slot]++;
		count.slotTiles[slot] += numTiles;

		cfg++;
		cfgSize--;
	}

	return true;
}

unsigned AssetUtil::totalTilesForPhysicalSlot(_SYSCubeID cid, unsigned slot)
{
	ASSERT(cid < _SYS_NUM_CUBE_SLOTS);
	ASSERT(slot < SysLFS::ASSET_SLOTS_PER_CUBE);

    SysLFS::Key cubeKey = CubeSlots::instances[cid].getCubeRecordKey();
    SysLFS::Key slotKey = SysLFS::AssetSlotRecord::makeKey(cubeKey, slot);
    SysLFS::AssetSlotRecord slotRecord;

    if (SysLFS::read(slotKey, slotRecord))
        return slotRecord.totalTiles();

    // Slot not allocated
    return 0;
}

bool AssetGroupInfo::fromUserPointer(const _SYSAssetGroup *group)
{
	/*
	 * 'group' is a userspace pointer, in the current volume.
	 * We raise a fault and return false on error.
	 *
	 * Here we map the group header from flash and read metadata out of it.
	 */

	// These groups never use volume remapping
	remapToVolume = false;

    // Capture user pointer prior to mapRAM
    va = reinterpret_cast<SvmMemory::VirtAddr>(group);

    if (!isAligned(group)) {
        SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
        return false;
    }
    if (!SvmMemory::mapRAM(group)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return false;
    }

    SvmMemory::VirtAddr localHeaderVA = group->pHdr;
    headerVA = localHeaderVA;

    _SYSAssetGroupHeader header;
    if (!SvmMemory::copyROData(header, localHeaderVA)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return false;
    }

    dataSize = header.dataSize;
    numTiles = header.numTiles;
    ordinal = header.ordinal;

	volume = SvmLoader::volumeForVA(localHeaderVA);
    if (!volume.block.isValid()) {
        SvmRuntime::fault(F_SYSCALL_PARAM);
        return false;
    }

    return true;
}

bool AssetGroupInfo::fromAssetConfiguration(const _SYSAssetConfiguration *config)
{
	/*
	 * The 'config' here has already been validated and mapped, but none of
	 * its contents can be trusted, as the AssetConfiguration is resident
	 * in userspace RAM.
	 *
	 * To further complicate things, alternate Volumes can be selected
	 * other than the one that's currently executing. (This is used for
	 * launcher icons, for example.)
	 *
	 * The division of labour between this function and AssetUtil::isValidConfig()
	 * is subtle. That function needs to report the easy errors in very obvious
	 * ways, to help userspace programmers debug their code. As such, it should
	 * check for things that are commonly mistaken. But it can't do anything to
	 * protect the system, since userspace could simply modify the config after
	 * loading has started. So, this function needs to protect the system. But
	 * it doesn't need to be particularly friendly, since at this point we should
	 * only be hitting very obscure bugs or intentionally malicious code.
	 *
	 * On error, we return 'false' but do NOT raise any faults. We just log them.
	 */

	// Must read these userspace fields exactly once
	SvmMemory::VirtAddr groupVA = config->pGroup;
	_SYSVolumeHandle volHandle = config->volume;

	// Save original group VA
	va = groupVA;

	// Local copy of _SYSAssetGroup itself
	_SYSAssetGroup group;
	if (!SvmMemory::copyROData(group, groupVA)) {
		LOG(("ASSET: Bad group pointer 0x%08x in _SYSAssetConfiguration\n", unsigned(groupVA)));
		return false;
	}

	// Save information originally from the header, copied to 'config' by userspace
	dataSize = config->dataSize;
	numTiles = config->numTiles;
	ordinal = config->ordinal;

	// Save header VA, from the _SYSAssetGroup
	SvmMemory::VirtAddr localHeaderVA = group.pHdr;
	headerVA = localHeaderVA;

	if (volHandle) {
		/*
		 * A volume was specified. It needs to be valid, and the header VA must
	     * be inside SVM Segment 1. We'll treat such a pointer as an offset into
	     * the proper volume as if that volume was mapped into Segment 1.
	     */

	    FlashVolume localVolume(volHandle);
	    if (!localVolume.isValid()) {
	    	LOG(("ASSET: Bad volume handle 0x%08x in _SYSAssetConfiguration\n", volHandle));
	    	return false;
	    }

	    uint32_t offset = localHeaderVA - SvmMemory::SEGMENT_1_VA;
	    if (offset >= FlashMap::NUM_BYTES - sizeof(_SYSAssetGroupHeader)) {
	    	LOG(("ASSET: Header VA 0x%08x in _SYSAssetConfiguration not in SEGMENT_1\n",
	    		 unsigned(localHeaderVA)));
	    	return false;
	    }

	    volume = localVolume;
	    remapToVolume = true;

	} else {
		/*
		 * No volume specified. The VA can be anything, we'll treat it like a
		 * normal address. Note that we need to explicitly remember that no
		 * remapping is occurring, so that future reads of asset data from
		 * Segment 1 come from the actual mapped volume, and not our substitute
		 * fake mapping :)
		 */

	    FlashVolume localVolume = SvmLoader::volumeForVA(localHeaderVA);
	    if (!localVolume.block.isValid()) {
	       	LOG(("ASSET: Bad local header VA 0x%08x in _SYSAssetConfiguration\n",
	       		unsigned(localHeaderVA)));
	    	return false;
	    }

	    volume = localVolume;
	    remapToVolume = false;
	}

	return true;
}
