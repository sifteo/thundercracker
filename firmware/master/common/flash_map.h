/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * The third layer of the flash stack: The flash chip is divided into
 * relatively large "mapping blocks", which can be discontiguously
 * arranged into a FlashMap.
 */

#ifndef FLASH_MAP_H_
#define FLASH_MAP_H_

#include "machine.h"
#include "flash_blockcache.h"
#include "flash_device.h"


/**
 * One of our coarse-grained flash mapping blocks. There are relatively
 * few of these in total, so we can refer to them using small 8-bit IDs.
 *
 * Keeping these blocks large lets us keep the IDs small, and in turn keeps
 * the FlashMap's time and space requirements under control.
 *
 * We need 0 to represent an invalid ID, so that we can mark blocks as
 * invalid (deleted) without erasing the block containing our Map.
 */
class FlashMapBlock
{
public:
    static const unsigned BLOCK_SIZE = 128 * 1024;  // Power of two
    static const unsigned BLOCK_MASK = BLOCK_SIZE - 1;
    static const unsigned NUM_BLOCKS = FlashDevice::CAPACITY / BLOCK_SIZE;

    // A BitVector with one bit for every possible FlashMapBlock index
    typedef BitVector<NUM_BLOCKS> Set;

    unsigned address() const {
        STATIC_ASSERT(NUM_BLOCKS <= (1ULL << (sizeof(code) * 8)));
        return index() * BLOCK_SIZE;
    }

    unsigned index() const {
        ASSERT(isValid());
        return code - 1;
    }

    bool isValid() const {
        return code != 0;
    }

    void setInvalid() {
        code = 0;
    }

    void setIndex(unsigned i) {
        code = i + 1;
    }

    void mark(Set &v) const {
        if (isValid())
            v.mark(index());
    }

    void clear(Set &v) const {
        if (isValid())
            v.clear(index());
    }

    static FlashMapBlock fromIndex(unsigned i) {
        FlashMapBlock result;
        result.setIndex(i);
        return result;
    }

    static FlashMapBlock invalid() {
        FlashMapBlock result;
        result.setInvalid();
        return result;
    }

    /// Synonymous with isValid()
    operator bool() const { return isValid(); }

    uint8_t code;     // invalid=0 , valid=[1, NUM_BLOCKS]
};


/**
 * A FlashMap describes a virtual-to-physical mapping for an object
 * made up of one or more FlashMapBlocks, up to the maximum object size.
 *
 * Maps are relatively heavy-weight objects. They require hundreds of
 * bytes of RAM, and creating a FlashMap requires scanning the block
 * headers in our flash memory.
 *
 * Lookups are typically not made directly through a FlashMap:
 * A FlashMapSpan is used to describe some bounded region of space within the
 * map, which is typically smaller than the maximum supported number of bytes.
 * The FlashMapSpan is responsible for bounds-checking, as well as applying
 * any offsets from the beginning of the first FlashMapBlock.
 */
class FlashMap
{
public:
    static const unsigned NUM_BYTES = 0x1000000;   // 24-bit limit imposed by SVM ISA
    static const unsigned NUM_MAP_BLOCKS = NUM_BYTES / FlashMapBlock::BLOCK_SIZE;
    static const unsigned NUM_CACHE_BLOCKS = NUM_BYTES / FlashBlock::BLOCK_SIZE;

    FlashMapBlock blocks[NUM_MAP_BLOCKS];

    // Convert a single FlashMapBlock pointer into a FlashMap pointer.
    static const FlashMap *single(const FlashMapBlock *block) {
        STATIC_ASSERT(offsetof(FlashMap, blocks) == 0);
        return reinterpret_cast<const FlashMap*>(block);
    }

    void mark(FlashMapBlock::Set &v) const;
    void clear(FlashMapBlock::Set &v) const;
};


/**
 * A range of virtual addresses within a FlashMap. These are lightweight
 * objects that reference an externally-stored FlashMap. FlashMapSpans
 * need not be aligned to the beginning of FlashMapBlocks, but they *are*
 * aligned to FlashBlock boundaries.
 *
 * FlashMapSpans are the lower-level objects that back individual virtual
 * address spaces in SVM.
 */
class FlashMapSpan
{
public:
    const FlashMap *map;
    uint16_t firstBlock;
    uint16_t numBlocks;

    // We *do* want ByteOffset to be 32-bit. It is important that we discard
    // extraneous high-bits on 64-bit hosts when translating flash addresses.

    typedef uint32_t ByteOffset;    // Number of bytes from the beginning of the span
    typedef uint32_t FlashAddr;     // Low-level flash address, in bytes
    typedef uint8_t* PhysAddr;      // Physical RAM address, in the block cache

    /**
     * Initialize the FlashMapSpan to a particular range of the
     * FlashMap, specified in units of cache blocks.
     */
    static FlashMapSpan create(const FlashMap *map,
        unsigned firstBlock, unsigned numBlocks)
    {
        STATIC_ASSERT(FlashMap::NUM_CACHE_BLOCKS <= (1ULL << (sizeof(firstBlock) * 8)));
        STATIC_ASSERT(sizeof firstBlock == sizeof numBlocks);
        FlashMapSpan result = { map, firstBlock, numBlocks };
        ASSERT(firstBlock < FlashMap::NUM_CACHE_BLOCKS);
        ASSERT(numBlocks < FlashMap::NUM_CACHE_BLOCKS);
        ASSERT(result.firstBlock == firstBlock);
        ASSERT(result.numBlocks == numBlocks);
        return result;
    }

    /// Return an empty FlashMapSpan. It contains no mapped blocks.
    static FlashMapSpan empty()
    {
        FlashMapSpan result = { 0, 0, 0 };
        return result;
    }

    /// Is there any valid block in this span?
    bool isEmpty() const {
        return numBlocks == 0;
    }

    uint32_t sizeInBytes() const {
        return (uint32_t)numBlocks * FlashBlock::BLOCK_SIZE;
    }

    uint32_t firstByte() const {
        return (uint32_t)firstBlock * FlashBlock::BLOCK_SIZE;
    }

    bool offsetIsValid(ByteOffset byteOffset) const {
        return byteOffset < sizeInBytes();
    }

    /// Split off a portion of this FlashMapSpan as a new span.
    FlashMapSpan split(unsigned blockOffset,
        unsigned blockCount = FlashMap::NUM_CACHE_BLOCKS) const;

    /// Range specified in bytes. Offset must be aligned, length is rounded up.
    FlashMapSpan splitRoundingUp(unsigned byteOffset, unsigned byteCount) const;

    // Translation functions
    bool flashAddrToOffset(FlashAddr flashAddr, ByteOffset &byteOffset) const;
    bool offsetToFlashAddr(ByteOffset byteOffset, FlashAddr &flashAddr) const;

    // Cached data access
    bool getBlock(FlashBlockRef &ref, ByteOffset byteOffset) const;
    bool getBytes(FlashBlockRef &ref, ByteOffset byteOffset, PhysAddr &ptr, uint32_t &length) const;
    bool getByte(FlashBlockRef &ref, ByteOffset byteOffset, PhysAddr &ptr) const;
    bool copyBytes(FlashBlockRef &ref, ByteOffset byteOffset, uint8_t *dest, uint32_t length) const;
    bool copyBytes(ByteOffset byteOffset, uint8_t *dest, uint32_t length) const;
    bool preloadBlock(ByteOffset byteOffset) const;

    // Cache-bypassing data access
    bool copyBytesUncached(ByteOffset byteOffset, uint8_t *dest, uint32_t length) const;
};


#endif
