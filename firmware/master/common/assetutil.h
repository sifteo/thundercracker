/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _ASSETUTIL_H
#define _ASSETUTIL_H

#include <sifteo/abi.h>
#include "macros.h"
#include "svmmemory.h"


/**
 * AssetUtil is home to some random asset-related utilities that are not
 * specifically related to asset loading.
 */

class AssetUtil
{
public:

    unsigned loadedBaseAddr(SvmMemory::VirtAddr assetGroup, _SYSCubeId cid);

private:
    AssetUtil();  // Do not implement
};


#endif
