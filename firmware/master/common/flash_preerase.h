/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef FLASH_PREERASE_H_
#define FLASH_PREERASE_H_

#include "flash_recycler.h"

/**
 * Manages the process of pre-erasing blocks.
 * Callers can erase blocks as long as they have time to kill.
 * Results are immediately committed to the FlashEraseLog.
 */

class FlashBlockPreEraser {
public:
    FlashBlockPreEraser();

    bool next();

private:
    FlashEraseLog log;
    FlashBlockRecycler recycler;
};


#endif
