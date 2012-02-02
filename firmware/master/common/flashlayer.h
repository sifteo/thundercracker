/*
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef FLASH_LAYER_H_
#define FLASH_LAYER_H_

#include <sifteo.h>
#include <stdio.h>
#include <stdint.h>

class FlashLayer
{
public:
    static const unsigned NUM_BLOCKS = 2;
    static const unsigned BLOCK_SIZE = 512;

    static void init();
    static void* getRegionFromOffset(unsigned offset, unsigned len, unsigned *size);
    static void releaseRegionFromOffset(int offset);

private:
    struct CachedBlock {
        uint32_t address;
        char data[BLOCK_SIZE];
        // TODO - track multiple references? timestamp?
    }  __attribute__((aligned(sizeof(uint32_t))));

    static CachedBlock blocks[NUM_BLOCKS];
    static uint32_t freeBlocksMask;
    static uint32_t validBlocksMask;    // TODO: invalidation?

    // Try to find an existing cached block for the given address.
    static CachedBlock* getCachedBlock(uintptr_t address);
    static CachedBlock* getFreeBlock();
    
#ifdef SIFTEO_SIMULATOR
    struct Stats {
        unsigned hits;
        unsigned misses;
    };
    static Stats stats;
#endif

};

#endif // FLASH_LAYER_H_
