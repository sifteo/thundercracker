/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef FLASH_LAYER_H_
#define FLASH_LAYER_H_

#include "svmvalidator.h"
#include "systime.h"
#include <sifteo/machine.h>
#include <stdint.h>

#ifdef SIFTEO_SIMULATOR
#  define FLASHLAYER_STATS
#  define FLASHLAYER_STATS_ONLY(x)  x
#else
#  define FLASHLAYER_STATS_ONLY(x)
#endif

class FlashBlockRef;

struct FlashStats {
    unsigned blockHitSame;
    unsigned blockHitOther;
    unsigned blockMiss;
    unsigned blockTotal;
    unsigned streamBytes;
    unsigned globalRefcount;
    SysTime::Ticks timestamp;
};

extern FlashStats gFlashStats;


/**
 * A single flash block, fetched via a globally shared cache.
 * This is the general-purpose mechansim used to randomly access arbitrary
 * sized data items from flash.
 */
class FlashBlock
{
public:
    static const unsigned NUM_BLOCKS = 16;
    static const unsigned BLOCK_SIZE = 256;     // Power of two
    static const unsigned BLOCK_MASK = BLOCK_SIZE - 1;
    static const unsigned MAX_REFCOUNT = NUM_BLOCKS;
    static const uint32_t INVALID_ADDRESS = (uint32_t)-1;
    #define BLOCK_ALIGN __attribute__((aligned(256)))

private:
    friend class FlashBlockRef;

    uint32_t stamp;
    uint32_t address;
    uint16_t validCodeBytes;
    uint8_t refCount;

    static uint8_t mem[NUM_BLOCKS][BLOCK_SIZE];
    static FlashBlock instances[NUM_BLOCKS];
    static uint32_t referencedBlocksMap;
    static uint32_t latestStamp;

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
    
    /**
     * Quick predicate to check a physical address. Used only in simulation.
     */
#ifdef SIFTEO_SIMULATOR
    static bool isAddrValid(uintptr_t pa) {
        uintptr_t offset = reinterpret_cast<uint8_t*>(pa) - &mem[0][0];
        return offset < sizeof mem;
    }
#endif

    static void init();
    static void preload(uint32_t blockAddr);
    static void get(FlashBlockRef &ref, uint32_t blockAddr);
    static uint8_t *getByte(FlashBlockRef &ref, uint32_t address);
    static uint8_t *getBytes(FlashBlockRef &ref, uint32_t address, uint32_t &length);

private:
    inline void incRef() {
        ASSERT(refCount <= MAX_REFCOUNT);
        ASSERT(refCount == 0 || (referencedBlocksMap & bit()));
        ASSERT(refCount != 0 || !(referencedBlocksMap & bit()));

        if (!refCount++)
            referencedBlocksMap |= bit();

        FLASHLAYER_STATS_ONLY({
            gFlashStats.globalRefcount++;
            ASSERT(gFlashStats.globalRefcount <= MAX_REFCOUNT);
        })
    }

    inline void decRef() {
        ASSERT(refCount <= MAX_REFCOUNT);
        ASSERT(refCount != 0);
        ASSERT(referencedBlocksMap & bit());

        if (!--refCount)
            referencedBlocksMap &= ~bit();

        FLASHLAYER_STATS_ONLY({
            ASSERT(gFlashStats.globalRefcount > 0);
            gFlashStats.globalRefcount--;
        })
    }
    
    static FlashBlock *lookupBlock(uint32_t blockAddr);
    static FlashBlock *recycleBlock();
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
        block->incRef();
    }

    FlashBlockRef(const FlashBlockRef &r) : block(&*r) {
        block->incRef();
    }
    
    inline bool isHeld() const {
        if (block) {
            ASSERT(block->refCount != 0);
            ASSERT(block->refCount <= block->MAX_REFCOUNT);
            return true;
        }
        return false;
    }

    inline void set(FlashBlock *b) {
        if (isHeld())
            block->decRef();
        block = b;
        if (b)
            b->incRef();
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
        ASSERT(isHeld());
        return block;
    }

private:
    FlashBlock *block;
};


/**
 * A contiguous range of bytes in Flash. Has a beginning and an end.
 */
class FlashRange {
public:
    FlashRange() {}

    FlashRange(uint32_t address, uint32_t size)
        : address(address), size(size) {}

    inline void init(uint32_t address, uint32_t size) {
        this->address = address;
        this->size = size;
    }

    inline void clear() {
        this->size = 0;
    }

    inline uint32_t getAddress() const {
        return address;
    }

    inline uint32_t getSize() const {
        return size;
    }
    
    inline bool isEmpty() const {
        return size == 0;
    }
    
    inline bool isAligned() const {
        return (address & FlashBlock::BLOCK_MASK) == 0;
    }

    FlashRange split(uint32_t sliceOffset, uint32_t sliceSize) const;

private:
    uint32_t address;
    uint32_t size;
};


/**
 * A contiguous region of flash, not necessarily block-aligned, used as a
 * source for streaming data. This does not use the cache layer, since it's
 * assumed that we'll be reading a large object in mostly linear order
 * and we'd rather not pollute the cache with all of these blocks that will
 * not be reused.
 */
class FlashStream : public FlashRange {
public:
    FlashStream() {}

    FlashStream(const FlashRange &r)
        : FlashRange(r), offset(0) {}

    FlashStream(uint32_t address, uint32_t size)
        : FlashRange(address, size), offset(0) {}

    inline void init(uint32_t address, uint32_t size) {
        FlashRange::init(address, size);
        this->offset = 0;
    }

    inline void clear() {
        FlashRange::clear();
        this->offset = 0;
    }

    inline bool eof() const {
        ASSERT(offset <= getSize());
        return offset >= getSize();
    }

    inline uint32_t tell() const {
        return offset;
    }

    inline void seek(uint32_t o) {
        ASSERT(o <= getSize());
        offset = o;
    }

    inline uint32_t remaining() const {
        ASSERT(offset <= getSize());
        return getSize() - offset;
    }

    inline void advance(uint32_t bytes) {
        seek(bytes + tell());
    }

    // Reads at the current offset, does *not* auto-advance.
    uint32_t read(uint8_t *dest, uint32_t maxLength);

private:
    uint32_t offset;
};


/**
 * A buffer that, combined with FlashStream, provides buffered I/O with
 * zero copies in the common case.
 *
 * The buffer will pull data from flash as necessary, by calling read() on
 * the provided stream. If sufficient data is already present in the buffer,
 * no read() call will occur.
 *
 * Any time the underlying FlashStream object is init()'ed or seek()'ed, the
 * buffer must be reset().
 */
template <unsigned bufSize>
class FlashStreamBuffer {
public:
    inline void reset() {
        writePtr = readPtr = 0;
    }

    /// Returns NULL on unexpected EOF.
    uint8_t *read(FlashStream &stream, uint32_t length) {
        ASSERT(length < bufSize);
        uint32_t bufferedBytes = writePtr - readPtr;

        if (bufferedBytes < length) {
            // Need more data!
            memmove(data, data + readPtr, bufferedBytes);
            bufferedBytes += stream.read(data + bufferedBytes, bufSize - bufferedBytes);
            readPtr = 0;
            writePtr = bufferedBytes;
        }
        if (bufferedBytes < length) {
            // EOF
            return NULL;
        }

        uint8_t *result = data + readPtr;
        readPtr += length;
        stream.advance(length);
        return result;
    }

private:
    uint16_t writePtr, readPtr;
    uint8_t data[bufSize];
};


#endif // FLASH_LAYER_H_
