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
#include "tasks.h"


extern "C" {


void _SYS_asset_bindSlots(_SYSVolumeHandle volHandle, unsigned numSlots)
{
    FlashVolume vol(volHandle);
    if (!vol.isValid())
        return SvmRuntime::fault(F_BAD_VOLUME_HANDLE);

    if (numSlots > VirtAssetSlots::NUM_SLOTS)
        return SvmRuntime::fault(F_SYSCALL_PARAM);

    VirtAssetSlots::bind(vol, numSlots);
}

uint32_t _SYS_asset_slotTilesFree(_SYSAssetSlot slot, _SYSCubeIDVector cv)
{
    if (!VirtAssetSlots::isSlotBound(slot)) {
        SvmRuntime::fault(F_BAD_ASSETSLOT);
        return 0;
    }

    cv = CubeSlots::truncateVector(cv);

    //return VirtAssetSlots::getInstance(slot).tilesFree(cv);
    return 0;
}

void _SYS_asset_slotErase(_SYSAssetSlot slot, _SYSCubeIDVector cv)
{
    if (!VirtAssetSlots::isSlotBound(slot))
        return SvmRuntime::fault(F_BAD_ASSETSLOT);

    cv = CubeSlots::truncateVector(cv);

    //VirtAssetSlots::getInstance(slot).erase(cv);
}

void _SYS_asset_loadStart(_SYSAssetLoader *loader,
    const struct _SYSAssetConfiguration *cfg, unsigned cfgSize, _SYSCubeIDVector cv)
{
    if (!isAligned(loader))
        return SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
    if (!SvmMemory::mapRAM(loader))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);
}

void _SYS_asset_loadFinish(_SYSAssetLoader *loader)
{
    if (!isAligned(loader))
        return SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
    if (!SvmMemory::mapRAM(loader))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);
}

void _SYS_asset_loadCancel(_SYSAssetLoader *loader, _SYSCubeIDVector cv)
{
    if (!isAligned(loader))
        return SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
    if (!SvmMemory::mapRAM(loader))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);
    
    cv = CubeSlots::truncateVector(cv);
}

uint32_t _SYS_asset_findInCache(_SYSAssetGroup *group, _SYSCubeIDVector cv)
{
    /*
     * Find this asset group in the cache, for all cubes marked in 'cv'.
     * Return a _SYSCubeIDVector of all cubes for whom this asset group
     * is already loaded. For each of these cubes, the _SYSAssetGroupCube's
     * baseAddr is updated.
     */

    MappedAssetGroup map;
    if (!map.init(group))
        return 0;

    cv = CubeSlots::truncateVector(cv);

    _SYSCubeIDVector cachedCV;
    if (!VirtAssetSlots::locateGroup(map, cv, cachedCV))
        cachedCV = 0;

    return cachedCV;
}


}  // extern "C"
