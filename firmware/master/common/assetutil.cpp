/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "assetutil.h"
#include "assetslot.h"


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

		if (!SvmMemory::mapRAM(groupVA, sizeof(_SYSAssetGroup), groupPA))
			return false;

		if (slot >= _SYS_ASSET_SLOTS_PER_BANK)
			return false;

	    if (!VirtAssetSlots::isSlotBound(slot))
	    	return false;

		if (count.slotGroups[slot] >= _SYS_ASSET_GROUPS_PER_SLOT)
			return false;

		if (unsigned(count.slotTiles[slot]) + numTiles >= _SYS_TILES_PER_ASSETSLOT)
			return false;

		count.slotGroups[slot]++;
		count.slotTiles[slot] += numTiles;

		cfg++;
		cfgSize--;
	}

	return true;
}
