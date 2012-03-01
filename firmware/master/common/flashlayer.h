/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef FLASH_LAYER_H_
#define FLASH_LAYER_H_

#include "svmvalidator.h"
#include <sifteo/machine.h>
#include <stdint.h>

#ifdef SIFTEO_SIMULATOR
#  define FLASHLAYER_STATS
#  define FLASHLAYER_STATS_ONLY(x)  x
#else
#  define FLASHLAYER_STATS_ONLY(x)
#endif

class FlashBlockRef;


class FlashBlock
{
public:
    static const unsigned NUM_BLOCKS = 16;
    static const unsigned BLOCK_SIZE = 256;     // Power of two
    static const unsigned BLOCK_MASK = BLOCK_SIZE - 1;
    static const uint32_t INVALID_ADDRESS = (uint32_t)-1;
    #define BLOCK_ALIGN __attribute__((aligned(256)))

private:
    friend class FlashBlockRef;

    struct Stats {
#ifdef FLASHLAYER_STATS
        unsigned hit_same;
        unsigned hit_other;
        unsigned miss;
        unsigned total;
        unsigned global_refcount;
#endif
    };

    uint32_t address;
    uint16_t validCodeBytes;
    uint8_t refCount;

    static uint8_t mem[NUM_BLOCKS][BLOCK_SIZE];
    static FlashBlock instances[NUM_BLOCKS];
    static uint32_t referencedBlocksMap;
    static uint32_t busyBlocksMap;
    static Stats stats;

public:
    inline unsigned id() {
        return (unsigned)(this - instances);
    }
    
    inline unsigned bit() {
        return Sifteo::Intrinsic::LZ(id());
    }

    inline uint32_t getAddress() {
        return address;
    }
    
    inline uint8_t *getData() {
        return &mem[id()][0];
    }

    inline bool isCodeOffsetValid(unsigned offset) {
        // Misaligned offsets are never valid
        if (offset & 3)
            return false;
        
        // Lazily validate
        if (validCodeBytes == 0)
            validCodeBytes = SvmValidator::validBytes(getData(), BLOCK_SIZE);

        return offset < validCodeBytes;
    }

    static void init();
    static void get(FlashBlockRef &ref, uint32_t blockAddr);
    static uint8_t *getBytes(FlashBlockRef &ref, uint32_t address, uint32_t &length);

private:
    inline void incRef() {
        assert(refCount < 0xFF);
        assert(refCount == 0 || (referencedBlocksMap & bit()));
        assert(refCount != 0 || !(referencedBlocksMap & bit()));

        if (!refCount++)
            referencedBlocksMap |= bit();

        FLASHLAYER_STATS_ONLY({
            stats.global_refcount++;
            ASSERT(stats.global_refcount <= NUM_BLOCKS);
        })
    }

    inline void decRef() {
        assert(refCount != 0);
        assert(referencedBlocksMap & bit());

        if (!--refCount)
            referencedBlocksMap &= ~bit();

        FLASHLAYER_STATS_ONLY({
            ASSERT(stats.global_refcount > 0);
            stats.global_refcount--;
        })
    }
    
    static FlashBlock *lookupBlock(uint32_t blockAddr);
    static FlashBlock *recycleBlock(uint32_t blockAddr);
};


class FlashBlockRef
{
public:
    FlashBlockRef() : block(0) {}

    FlashBlockRef(FlashBlock *block) : block(block) {
        block->incRef();
    }

    FlashBlockRef(const FlashBlockRef &r) : block(&*r) {
        block->incRef();
    }
    
    inline bool isHeld() const {
        return block != 0;
    }

    inline void set(FlashBlock *b) {
        if (isHeld())
            block->decRef();
        block = b;
    }
    
    inline void release() {
        set(0);
    }

    ~FlashBlockRef() {
        release();
    }

    inline FlashBlock& operator*() const {
        return *block;
    }

    inline FlashBlock* operator->() const {
        return block;
    }

private:
    FlashBlock *block;
};


#endif // FLASH_LAYER_H_
