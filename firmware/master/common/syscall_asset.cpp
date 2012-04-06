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

extern "C" {


uint32_t _SYS_asset_slotTilesFree(_SYSAssetSlot slot)
{
    return 0;
}

void _SYS_asset_slotErase(_SYSAssetSlot slot)
{
}

uint32_t _SYS_asset_loadStart(_SYSAssetLoader *loader, _SYSAssetGroup *group,
    _SYSAssetSlot slot, _SYSCubeIDVector cv)
{
    // XXX

    if (!SvmMemory::mapRAM(loader, sizeof *loader))
        return false;
    CubeSlots::assetLoader = loader;

    while (cv) {
        _SYSCubeID id = Intrinsic::CLZ(cv);
        cv ^= Intrinsic::LZ(id);
        CubeSlot &cube = CubeSlots::instances[id];

        cube.startAssetLoad(reinterpret_cast<SvmMemory::VirtAddr>(group), 0);
    }

    return true;
}

void _SYS_asset_loadFinish(_SYSAssetLoader *loader)
{
    CubeSlots::assetLoader = NULL;
}

uint32_t _SYS_asset_findInCache(_SYSAssetGroup *group)
{
    return false;
}


}  // extern "C"
