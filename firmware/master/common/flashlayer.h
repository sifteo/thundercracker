/*
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef FLASH_LAYER_H_
#define FLASH_LAYER_H_

#include <stdint.h>

class FlashLayer
{
public:
    static const int NUM_BLOCKS = 2;
    static const int BLOCK_SIZE = 512; // XXX - HW dependent

    static char* getRegion(uint32_t address, int len);
    static void releaseRegion(uint32_t address);

private:
    typedef struct CachedBlock_t {
        char data[BLOCK_SIZE];
        uint32_t address;
        // TODO - track multiple references? timestamp?
    } CachedBlock;

#ifdef SIFTEO_SIMULATOR
    static int hits;
    static int misses;
#endif
    static CachedBlock blocks[NUM_BLOCKS];
//    static uint8_t freeBlocksMask = 0;      // bitmask indicating which blocks are in use

    static CachedBlock* getCachedBlock(uint32_t address);
};

#endif // FLASH_LAYER_H_
