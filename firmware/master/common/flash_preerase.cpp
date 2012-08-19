/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_preerase.h"


// Tell our FlashBlockRecycler not to use the erase log
FlashBlockPreEraser::FlashBlockPreEraser()
    : recycler(false)
{}

bool FlashBlockPreEraser::next()
{
    /*
     * Recycle and log one more block.
     *
     * We asked the Recycler to bypass using the erase log, otherwise
     * we would not be guaranteed to make forward progress here.
     */

    if (!log.allocate(recycler))
        return false;

    FlashEraseLog::Record r;
    if (!recycler.next(r.block, r.ec))
        return false;

    log.commit(r);
    return true;
}
