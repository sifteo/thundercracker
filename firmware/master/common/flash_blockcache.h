/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * The second layer of the flash stack: Cached access to physical
 * flash blocks. This layer knows nothing of virtual-to-physical address
 * translation, only of retrieving and caching physical blocks.
 */

#ifndef FLASH_BLOCKCACHE_H_
#define FLASH_BLOCKCACHE_H_

#include "flash_device.h"
#include "svmvalidator.h"
#include "systime.h"
#include "machine.h"
#include "macros.h"
#include <stdint.h>
#include <string.h>

#ifdef SIFTEO_SIMULATOR
#  define FLASHLAYER_STATS_ONLY(x)  x
#else
#  define FLASHLAYER_STATS_ONLY(x)
#endif

class FlashBlockRef;
class FlashBlockWriter;

/**
 * A single flash block, fetched via a globally shared cache.
 * This is the general-purpose mechansim used to randomly access arbitrary
 * sized data items from flash.
 */
class FlashBlock
{
public:
    static const unsigned NUM_CACHE_BLOCKS = 16;
    static const unsigned BLOCK_SIZE = 256;     // Power of two
    static const unsigned BLOCK_MASK = BLOCK_SIZE - 1;
    static const unsigned MAX_REFCOUNT = NUM_CACHE_BLOCKS;
    static const uint32_t INVALID_ADDRESS = (uint32_t)-1;
    #define BLOCK_ALIGN __attribute__((aligned(256)))

    /// Flags
    enum {
        F_KNOWN_ERASED = (1 << 0),      // Contents known to be erased
    };

private:
    friend class FlashBlockRef;
    friend class FlashBlockWriter;

    uint32_t stamp;
    uint32_t address;
    uint16_t validCodeBytes;
    uint8_t refCount;

    struct FlashStats {
        bool enabled;
        unsigned globalRefcount;
        SysTime::Ticks timestamp;

        // These counters are reset on every interval
        struct {
            unsigned blockHitSame;
            unsigned blockHitOther;
            unsigned blockMiss;
            unsigned blockTotal;

            // Should be last, for efficiency. This is large!
            uint32_t blockMissCounts[FlashDevice::CAPACITY / BLOCK_SIZE];
        } periodic;
    };

    FLASHLAYER_STATS_ONLY(static FlashStats stats;)

    static uint8_t mem[NUM_CACHE_BLOCKS][BLOCK_SIZE] SECTION(".blockcache");
    static FlashBlock instances[NUM_CACHE_BLOCKS];
    static uint32_t referencedBlocksMap;
    static uint32_t latestStamp;

public:
    inline unsigned id() {
        return (unsigned)(this - instances);
    }
    
    inline unsigned bit() {
        return Intrinsic::LZ(id());
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

#ifdef SIFTEO_SIMULATOR
    static bool isAddrValid(uintptr_t pa);
    static void enableStats();
    static void resetStats();
    static void dumpStats();
    static bool hotBlockSort(unsigned i, unsigned j);
    static void countBlockMiss(uint32_t blockAddr);
    void verify();
#endif

    // Global operations
    static void init();
    static void invalidate();

    // Cached block accessors
    static void preload(uint32_t blockAddr);
    static void cacheEraseSector(uint32_t sectorAddr);
    static void get(FlashBlockRef &ref, uint32_t blockAddr, unsigned flags = 0);

    // Support for anonymous memory
    static void anonymous(FlashBlockRef &ref);
    static void anonymous(FlashBlockRef &ref, uint8_t fillByte);

private:
    inline void incRef() {
        ASSERT(refCount <= MAX_REFCOUNT);
        ASSERT(refCount == 0 || (referencedBlocksMap & bit()));
        ASSERT(refCount != 0 || !(referencedBlocksMap & bit()));

        if (!refCount++)
            referencedBlocksMap |= bit();

        FLASHLAYER_STATS_ONLY({
            stats.globalRefcount++;
            ASSERT(stats.globalRefcount <= MAX_REFCOUNT);
        })
    }

    inline void decRef() {
        ASSERT(refCount <= MAX_REFCOUNT);
        ASSERT(refCount != 0);
        ASSERT(referencedBlocksMap & bit());

        if (!--refCount)
            referencedBlocksMap &= ~bit();

        FLASHLAYER_STATS_ONLY({
            ASSERT(stats.globalRefcount > 0);
            stats.globalRefcount--;
        })
    }
    
    static FlashBlock *lookupBlock(uint32_t blockAddr);
    static FlashBlock *recycleBlock();
    void load(uint32_t blockAddr, unsigned flags = 0);
};


/**
 * A reference to a single cached flash block. While the reference is held,
 * the block will be maintained in the cache. These objects can be used
 * transiently during a single memory operation, or they can be held for
 * longer periods of time.
 */
class FlashBlockRef
{
public:
    FlashBlockRef() : block(0) {}

    FlashBlockRef(FlashBlock *block) : block(block) {
        ASSERT(block);
        block->incRef();
    }

    FlashBlockRef(const FlashBlockRef &r) : block(r.block) {
        if (block)
            block->incRef();
    }

    ~FlashBlockRef() {
        release();
    }

    bool isHeld() const {
        if (block) {
            ASSERT(block->refCount != 0);
            ASSERT(block->refCount <= block->MAX_REFCOUNT);
            return true;
        }
        return false;
    }

    void set(FlashBlock *b) {
        if (isHeld())
            block->decRef();
        block = b;
        if (b)
            b->incRef();
    }
    
    void release() {
        set(0);
        ASSERT(!isHeld());
    }

    FlashBlock& operator*() const {
        return *block;
    }

    FlashBlock* operator->() const {
        ASSERT(isHeld());
        return block;
    }

    FlashBlockRef& operator=(const FlashBlockRef &r) {
        block = r.block;
        if (block)
            block->incRef();
        return *this;
    }

private:
    FlashBlock *block;
};


/**
 * A utility class to represent writes-in-progress to a single flash block.
 *
 * The FlashBlockWriter instance can be in either the 'clean' state, where
 * no writes are pending, or the 'dirty' state, in which we have a single
 * page in the cache which needs to be written to the device.
 *
 * Before the write starts,  we perform some invalidation and checking.
 * Then we can complete the write by actually flushing a dirty cache block
 * to the device.
 */

class FlashBlockWriter
{
public:
    FlashBlockWriter() {}

    FlashBlockWriter(const FlashBlockRef &r) {
        beginBlock(r);
    }

    ~FlashBlockWriter() {
        commitBlock();
    }

    // Dirty iff ref.isHeld()
    FlashBlockRef ref;

    void beginBlock();
    void beginBlock(uint32_t blockAddr);
    void beginBlock(const FlashBlockRef &r);

    void relocate(uint32_t blockAddr);

    void commitBlock();

    template <typename T>
    T *getData(uint32_t address) {
        unsigned blockPart = address & ~FlashBlock::BLOCK_MASK;
        unsigned bytePart = address & FlashBlock::BLOCK_MASK;
        ASSERT(bytePart + sizeof(T) <= FlashBlock::BLOCK_SIZE);

        beginBlock(blockPart);
        return reinterpret_cast<T*>(ref->getData() + bytePart);
    }
};


#endif
