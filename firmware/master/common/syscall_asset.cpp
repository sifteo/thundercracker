/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for asset group and slot operations.
 */

#include <sifteo/abi.h>
#include "svmmemory.h"
#include "cube.h"
#include "cubeslots.h"
#include "svmruntime.h"
#include "flash_syslfs.h"
#include "assetslot.h"
#include "assetloader.h"
#include "assetutil.h"
#include "tasks.h"
#include "svmloader.h"


extern "C" {


void _SYS_asset_bindSlots(_SYSVolumeHandle volHandle, unsigned numSlots)
{
    FlashVolume vol(volHandle);
    if (!vol.isValid())
        return SvmRuntime::fault(F_BAD_VOLUME_HANDLE);
    if (numSlots > VirtAssetSlots::NUM_SLOTS)
        return SvmRuntime::fault(F_SYSCALL_PARAM);

    AssetLoader::init();
    VirtAssetSlots::bind(vol, numSlots);
}

uint32_t _SYS_asset_slotTilesFree(_SYSAssetSlot slot, _SYSCubeIDVector cv)
{
    if (!VirtAssetSlots::isSlotBound(slot)) {
        SvmRuntime::fault(F_BAD_ASSETSLOT);
        return 0;
    }
    cv = CubeSlots::truncateVector(cv);

    return VirtAssetSlots::getInstance(slot).tilesFree(cv);
}

void _SYS_asset_slotErase(_SYSAssetSlot slot, _SYSCubeIDVector cv)
{
    if (!VirtAssetSlots::isSlotBound(slot))
        return SvmRuntime::fault(F_BAD_ASSETSLOT);
    cv = CubeSlots::truncateVector(cv);

    VirtAssetSlots::getInstance(slot).erase(cv);
}

void _SYS_asset_loadStart(_SYSAssetLoader *loader,
    const struct _SYSAssetConfiguration *cfg, unsigned cfgSize, _SYSCubeIDVector cv)
{
    if (!isAligned(loader))
        return SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
    if (!SvmMemory::mapRAM(loader))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    if (!isAligned(cfg))
        return SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
    if (!SvmMemory::mapRAM(cfg, mulsat16x16(cfgSize, sizeof *cfg)))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);
    if (!AssetUtil::isValidConfig(cfg, cfgSize))
        return SvmRuntime::fault(F_BAD_ASSET_CONFIG);

    _SYSAssetLoader *prevLoader = AssetLoader::getUserLoader();
    if (prevLoader && prevLoader != loader)
        return SvmRuntime::fault(F_BAD_ASSET_LOADER);

    cv = CubeSlots::truncateVector(cv);

    AssetLoader::start(loader, cfg, cfgSize, cv);
}

void _SYS_asset_loadFinish(_SYSAssetLoader *loader)
{
    if (!isAligned(loader))
        return SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
    if (!SvmMemory::mapRAM(loader))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);
    if (AssetLoader::getUserLoader() != loader)
        return SvmRuntime::fault(F_BAD_ASSET_LOADER);

    AssetLoader::finish();
}

void _SYS_asset_loadCancel(_SYSAssetLoader *loader, _SYSCubeIDVector cv)
{
    if (!isAligned(loader))
        return SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
    if (!SvmMemory::mapRAM(loader))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);
    if (AssetLoader::getUserLoader() != loader)
        return SvmRuntime::fault(F_BAD_ASSET_LOADER);
    
    cv = CubeSlots::truncateVector(cv);

    AssetLoader::cancel(cv);
}

uint32_t _SYS_asset_findInCache(_SYSAssetGroup *group, _SYSCubeIDVector cv)
{
    /*
     * Find this asset group in the cache, for all cubes marked in 'cv'.
     * Return a _SYSCubeIDVector of all cubes for whom this asset group
     * is already loaded. For each of these cubes, the _SYSAssetGroupCube's
     * baseAddr is updated.
     */

    if (!isAligned(group)) {
        SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
        return 0;
    }
    if (!SvmMemory::mapRAM(group)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return 0;
    }
    
    SvmMemory::VirtAddr hdrVA = group->pHdr;
    _SYSAssetGroupHeader header;

    if (!SvmMemory::copyROData(header, hdrVA)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return 0;
    }

    SysLFS::AssetGroupIdentity id;
    id.ordinal = header.ordinal;
    id.volume = SvmLoader::volumeForVA(hdrVA).block.code;

    if (!id.volume) {
        SvmRuntime::fault(F_SYSCALL_PARAM);
        return 0;
    }

    // We can only trust the cache if the AssetLoader has verified it.
    // (The process of querying this state requires AssetLoader's FIFO and Task)
    cv = cv & AssetLoader::getCacheCoherentCubes();
    if (!cv)
        return 0;

    _SYSCubeIDVector cachedCV;
    if (!VirtAssetSlots::locateGroup(header, id, cv, cachedCV))
        return 0;

    return cachedCV;
}


}  // extern "C"
