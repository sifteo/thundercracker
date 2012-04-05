/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for asset group and slot operations.
 */

#include <sifteo/abi.h>
#include "svmmemory.h"

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
    return 1;
}

void _SYS_asset_loadFinish(_SYSAssetLoader *loader)
{
}

uint32_t _SYS_asset_findInCache(_SYSAssetGroup *group)
{
    return 0;
}


}  // extern "C"
