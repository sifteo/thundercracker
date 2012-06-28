/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "macros.h"
#include "flash_stack.h"
#include "flash_blockcache.h"
#include "flash_lfs.h"


void FlashStack::init()
{
    FlashDevice::init();
    FlashBlock::init();
    FlashLFSCache::invalidate();
}


void FlashStack::invalidateCache()
{
    FlashBlock::invalidate();
    FlashLFSCache::invalidate();
}

