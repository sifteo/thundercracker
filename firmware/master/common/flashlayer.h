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

    static char* getRegion(uintptr_t address, int len);
    static char* getRegionFromOffset(int offset, int len, int *size);
    static void releaseRegion(uintptr_t address);
    static void releaseRegionFromOffset(int offset);

private:
    typedef struct CachedBlock_t {
        char data[BLOCK_SIZE];
        uint32_t address;
        // TODO - track multiple references? timestamp?
        // TODO: use a bitfield to track this state, get rid of inUse and valid
        bool inUse;
        bool valid;
    } CachedBlock;

#ifdef SIFTEO_SIMULATOR
    static int hits;
    static int misses;
#endif
    static CachedBlock blocks[NUM_BLOCKS];
//    static uint8_t freeBlocksMask = 0;      // bitmask indicating which blocks are in use

    static CachedBlock* getCachedBlock(uintptr_t address);
    static CachedBlock* getFreeBlock();
    
    
#ifdef SIFTEO_SIMULATOR
    // HACK: Attempt to redirect the flash layer at a file.  Need a proper
    //       hardware emulation layer, and firmware implementation
    
    static FILE * mFile;
    static bool mInit;
    
    static void init();
#endif

};

#endif // FLASH_LAYER_H_
