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

struct FlashRegion;

class FlashLayer
{
public:
    static const unsigned NUM_BLOCKS = 16;
    static const unsigned BLOCK_SIZE = 256;
    #define BLOCK_ALIGN __attribute__((aligned(256)))

    static void init();

    static bool getRegion(unsigned offset, unsigned len, FlashRegion *r);
    static void releaseRegion(const FlashRegion &r);

private:
    struct CachedBlock {
        uint32_t address;
        char data[BLOCK_SIZE];
        // TODO - track multiple references? timestamp?
    };

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

struct FlashRegion {
    FlashRegion() :
        _address(0xFFFFFFFF),   // default to invalid address
        _size(0),
        _data(0)
    {}

    ~FlashRegion() {
//        TODO: auto release?
//        FlashLayer::releaseRegion(*this);
    }

    unsigned baseAddress() const {
        return _address;
    }

    unsigned size() const {
        return _size;
    }

    void* data() const {
        return _data;
    }

    void reset() {
        _address = 0xFFFFFFFF;
        _size = 0;
        _data = 0;
    }

private:
    unsigned _address;
    unsigned _size;
    void *_data;

    friend class FlashLayer;
};

#endif // FLASH_LAYER_H_
