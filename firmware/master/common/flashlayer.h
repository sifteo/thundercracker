/*
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef FLASH_LAYER_H_
#define FLASH_LAYER_H_

#include <stdio.h>
#include <stdint.h>

class FlashLayer
{
public:
    static const int NUM_BLOCKS = 2;
    static const int BLOCK_SIZE = 512; // XXX - HW dependent

    static void init();
    static void* getRegionFromOffset(int offset, int len, int *size);
    static void releaseRegionFromOffset(int offset) {
        if (CachedBlock *b = getCachedBlock(offset)) {
            b->inUse = false;
        }
    }

private:
    typedef struct CachedBlock_t {
        char data[BLOCK_SIZE];
        uint32_t address;
        // TODO - track multiple references? timestamp?
        // TODO: use a bitfield to track this state, get rid of inUse and valid
        bool inUse;
        bool valid; // does this block contain valid data? TODO: invalidation?
    } CachedBlock;

    static CachedBlock blocks[NUM_BLOCKS];

    // Try to find an existing cached block for the given address.
    static CachedBlock* getCachedBlock(uintptr_t address) {
        CachedBlock *b;
        int i;
        for (i = 0, b = FlashLayer::blocks; i < NUM_BLOCKS; i++, b++) {
            if (address >= b->address && address < b->address + BLOCK_SIZE && b->valid) {
                return b;
            }
        }
        return 0;
    }

    static CachedBlock* getFreeBlock() {
        // TODO: get rid of inUse flag and use bitmask for more efficient lookup of
        //       free blocks.
        CachedBlock *b;
        int i;
        for (i = 0, b = FlashLayer::blocks; i < NUM_BLOCKS; i++, b++) {
            if (!b->inUse) {
                return b;
            }
        }
        return 0;
    }
    
#ifdef SIFTEO_SIMULATOR
    // HACK: Attempt to redirect the flash layer at a file.  Need a proper
    //       hardware emulation layer, and firmware implementation
    
    static FILE * mFile;
    struct Stats {
        unsigned hits;
        unsigned misses;
    };
    static struct Stats stats;
#endif

};

#endif // FLASH_LAYER_H_
