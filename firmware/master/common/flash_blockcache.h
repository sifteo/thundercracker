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
#include "svm.h"
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
    // Cache layout (Preferably a power of two)
    static const unsigned NUM_CACHE_BLOCKS = 64;    // 16 kB of cache
    static const unsigned MAX_REFCOUNT = NUM_CACHE_BLOCKS;

    // Block size (Must be a power of two)
    static const unsigned BLOCK_SIZE_LOG2 = 8;
    static const unsigned BLOCK_SIZE = 1 << BLOCK_SIZE_LOG2;
    static const unsigned BLOCK_MASK = BLOCK_SIZE - 1;
    #define BLOCK_ALIGN __attribute__((aligned(256)))

    // Special address for anonymous blocks
    static const uint32_t INVALID_ADDRESS = (uint32_t)-1;

    /// Flags
    enum {
        F_KNOWN_ERASED  = (1 << 0),      // Contents known to be erased
        F_ABORT_TRAP    = (1 << 1),      // Page is full of _SYS_abort() calls
    };

private:
    friend class FlashBlockRef;
    friend class FlashBlockWriter;

    // Keep this packed and power-of-two length
    uint32_t address;
    uint16_t stamp;
    uint8_t refCount;
    uint8_t idByte;

    struct FlashStats {
        unsigned globalRefcount;
        SysTime::Ticks timestamp;

        // These counters are reset on every interval
        struct {
            unsigned blockHitSame;
            unsigned blockHitOther;
            unsigned blockMiss;
            unsigned blockTotal;

            // Should be last, for efficiency. This is large!
            uint32_t blockMissCounts[FlashDevice::MAX_CAPACITY / BLOCK_SIZE];
        } periodic;
    };

    FLASHLAYER_STATS_ONLY(static FlashStats stats;)

    static uint8_t mem[NUM_CACHE_BLOCKS][BLOCK_SIZE] SECTION(".blockcache");
    static FlashBlock instances[NUM_CACHE_BLOCKS];
    static unsigned latestStamp;

    // Stored out-of-line, to keep the main FlashBlock length a power-of-two
    static uint8_t validCodeBundles[NUM_CACHE_BLOCKS];

public:
    ALWAYS_INLINE unsigned id() const {
        return idByte;
    }

    ALWAYS_INLINE uint32_t getAddress() const {
        return address;
    }

    ALWAYS_INLINE bool isAnonymous() const {
        return address == INVALID_ADDRESS;
    }

    ALWAYS_INLINE uint8_t *getData() const {
        return &mem[id()][0];
    }

    ALWAYS_INLINE bool isCodeOffsetValid(unsigned offset)
    {
        STATIC_ASSERT(BLOCK_SIZE == Svm::BLOCK_SIZE);
        STATIC_ASSERT(Svm::BUNDLE_SIZE == 4);
        STATIC_ASSERT(Svm::BUNDLE_SIZE * 0xFF >= Svm::BLOCK_SIZE);

        // Must be bundle-aligned
        if (offset & 3)
            return false;
        
        // Lazily validate
        uint8_t &pcb = validCodeBundles[id()];
        unsigned cb = pcb;

        if (cb == 0) {
            const uint32_t *words = reinterpret_cast<const uint32_t*>(getData());
            pcb = cb = SvmValidator::findValidBundles(words);
        }

        return (offset >> 2) < cb;
    }

#ifdef SIFTEO_SIMULATOR
    static bool isAddrValid(uintptr_t pa);
    static void resetStats();
    static void dumpStats();
    static bool hotBlockSort(unsigned i, unsigned j);
    static void countBlockMiss(uint32_t blockAddr);
    void verify();
#endif

    // Global operations
    static void init();
    static void invalidate(unsigned flags = 0);
    static void invalidate(uint32_t addrBegin, uint32_t addrEnd, unsigned flags = 0);

    // Cached block accessors
    static void preload(uint32_t blockAddr);
    static void get(FlashBlockRef &ref, uint32_t blockAddr, unsigned flags = 0);

    // Support for anonymous memory
    static void anonymous(FlashBlockRef &ref);
    static void anonymous(FlashBlockRef &ref, uint8_t fillByte);

    // Single-block invalidate
    void invalidateBlock(unsigned flags = 0);

private:
    ALWAYS_INLINE void incRef() {
        ASSERT(refCount <= MAX_REFCOUNT);

        refCount++;

        FLASHLAYER_STATS_ONLY({
            stats.globalRefcount++;
            ASSERT(stats.globalRefcount <= MAX_REFCOUNT);
        })
    }

    ALWAYS_INLINE void decRef() {
        ASSERT(refCount <= MAX_REFCOUNT);
        ASSERT(refCount != 0);

        refCount--;

        FLASHLAYER_STATS_ONLY({
            ASSERT(stats.globalRefcount > 0);
            stats.globalRefcount--;
        })
    }
    
    ALWAYS_INLINE unsigned getAge(unsigned latest) {
        return uint16_t(latest - stamp);
    }

    static FlashBlock *lookupBlock(uint32_t blockAddr);
    static FlashBlock *recycleBlock(uint32_t blockAddr);
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
    ALWAYS_INLINE FlashBlockRef() : block(0) {}

    ALWAYS_INLINE FlashBlockRef(FlashBlock *block) : block(block) {
        ASSERT(block);
        block->incRef();
    }

    ALWAYS_INLINE FlashBlockRef(const FlashBlockRef &r) : block(r.block) {
        if (block)
            block->incRef();
    }

    ALWAYS_INLINE ~FlashBlockRef() {
        release();
    }

    bool ALWAYS_INLINE isHeld() const {
        if (block) {
            ASSERT(block->refCount != 0);
            ASSERT(block->refCount <= block->MAX_REFCOUNT);
            return true;
        }
        return false;
    }

    void ALWAYS_INLINE set(FlashBlock *b) {
        if (isHeld())
            block->decRef();
        block = b;
        if (b)
            b->incRef();
    }
    
    void ALWAYS_INLINE release() {
        set(0);
        ASSERT(!isHeld());
    }

    ALWAYS_INLINE FlashBlock& operator*() const {
        return *block;
    }

    ALWAYS_INLINE FlashBlock* operator->() const {
        ASSERT(isHeld());
        return block;
    }

    ALWAYS_INLINE FlashBlockRef& operator=(const FlashBlockRef &r) {
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
 *
 * The writer automatically commits on destruction *unless* the block in
 * question is anonymous. If an anonymous write is cancelled, the data is
 * discarded. Writes with a specific address (not anonymous) cannot be
 * cancelled, since the cache block has already been modified.
 */

class FlashBlockWriter
{
public:
    ALWAYS_INLINE FlashBlockWriter() {}

    ALWAYS_INLINE FlashBlockWriter(const FlashBlockRef &r) {
        beginBlock(r);
    }

    ALWAYS_INLINE ~FlashBlockWriter() {
        if (ref.isHeld() && !ref->isAnonymous())
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
