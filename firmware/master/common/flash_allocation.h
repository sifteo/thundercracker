/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * The third layer of the flash stack: The flash chip is divided into
 * relatively large "allocation blocks". Each of these blocks may either
 * be unused, or may be allocated as part of an "allocation object".
 * Allocation objects may be ELF files, log-structured filesystems, or other
 * coarse-grained persistent objects.
 */

#ifndef FLASH_ALLOCATION_H_
#define FLASH_ALLOCATION_H_

#include "flash_blockcache.h"
#include "flash_device.h"


/**
 * One of our coarse-grained flash allocation blocks. There are relatively
 * few of these in total, so we can refer to them using small 8-bit IDs.
 *
 * Keeping these blocks large lets us keep the IDs small, and in turn keeps
 * the FlashAllocMap's time and space requirements under control.
 */
class FlashAllocBlock
{
public:
    static const unsigned BLOCK_SIZE = 128 * 1024;  // Power of two
    static const unsigned BLOCK_MASK = BLOCK_SIZE - 1;
    static const unsigned NUM_BLOCKS = FlashDevice::CAPACITY / BLOCK_SIZE;

    uint8_t id;     // 0 to NUM_BLOCKS - 1

    unsigned address() const {
        STATIC_ASSERT(NUM_BLOCKS <= (1ULL << (sizeof(id) * 8)));
        return id * BLOCK_SIZE;
    }
};


/**
 * An allocation map describes a virtual-to-physical mapping for an object
 * made up of one or more FlashAllocBlocks, up to the maximum object size.
 *
 * Allocation maps are relatively heavy-weight objects. They require hundreds
 * of bytes of RAM, and creating an Allocation Map requires scanning the block
 * headers in our flash memory.
 *
 * Lookups are typically not made directly through a FlashAllocMap:
 * A FlashAllocSpan is used to describe some bounded region of space within the
 * map, which is typically smaller than the maximum supported number of bytes.
 * The FlashAllocSpan is responsible for bounds-checking, as well as applying
 * any offsets from the beginning of the first FlashAllocBlock.
 */
class FlashAllocMap
{
public:
    static const unsigned NUM_BYTES = 0x1000000;   // 24-bit limit imposed by SVM ISA
    static const unsigned NUM_ALLOC_BLOCKS = NUM_BYTES / FlashAllocBlock::BLOCK_SIZE;
    static const unsigned NUM_CACHE_BLOCKS = NUM_BYTES / FlashBlock::BLOCK_SIZE;

    FlashAllocBlock blocks[NUM_ALLOC_BLOCKS];
};


/**
 * A range of virtual addresses within a FlashAllocMap. These are lightweight
 * objects that reference an externally-stored FlashAllocMap. FlashAllocSpans
 * need not be aligned to the beginning of FlashAllocBlocks, but they *are*
 * aligned to FlashBlock boundaries.
 *
 * FlashAllocSpans are the lower-level objects that back individual virtual
 * address spaces in SVM.
 */
class FlashAllocSpan
{
public:
    FlashAllocMap *map;
    uint16_t firstBlock;
    uint16_t numBlocks;

    typedef uintptr_t ByteOffset;   // Number of bytes from the beginning of the span
    typedef uint32_t FlashAddr;     // Low-level flash address, in bytes
    typedef uint8_t* PhysAddr;      // Physical RAM address, in the block cache

    /**
     * Initialize the FlashAllocSpan to a particular range of the
     * FlashAllocMap, specified in units of cache blocks.
     */
    static FlashAllocSpan create(FlashAllocMap *map,
        unsigned firstBlock, unsigned numBlocks)
    {
        STATIC_ASSERT(FlashAllocMap::NUM_CACHE_BLOCKS <= (1ULL << (sizeof(firstBlock) * 8)));
        STATIC_ASSERT(sizeof firstBlock == sizeof numBlocks);
        ASSERT(firstBlock < FlashAllocMap::NUM_CACHE_BLOCKS);
        ASSERT(numBlocks < FlashAllocMap::NUM_CACHE_BLOCKS);
        FlashAllocSpan result = { map, firstBlock, numBlocks };
        return result;
    }

    /// Return an empty FlashAllocSpan. It contains no mapped blocks.
    static FlashAllocSpan empty()
    {
        FlashAllocSpan result = { 0, 0, 0 };
        return result;
    }

    /// Is there any valid block in this span?
    bool isEmpty() const {
        return numBlocks == 0;
    }

    /// Split off a portion of this FlashAllocSpan as a new span.
    FlashAllocSpan split(unsigned blockOffset,
        unsigned blockCount = FlashAllocMap::NUM_CACHE_BLOCKS) const;

    // Translation functions
    bool flashAddrToOffset(FlashAddr flashAddr, ByteOffset &byteOffset) const;
    bool offsetToFlashAddr(ByteOffset byteOffset, FlashAddr &flashAddr) const;
    bool offsetIsValid(ByteOffset byteOffset) const;

    // Cached data access
    bool getBlock(FlashBlockRef &ref, ByteOffset byteOffset) const;
    bool getBytes(FlashBlockRef &ref, ByteOffset byteOffset, PhysAddr &ptr, uint32_t &length) const;
    bool getByte(FlashBlockRef &ref, ByteOffset byteOffset, PhysAddr &ptr) const;
    bool copyBytes(FlashBlockRef &ref, ByteOffset byteOffset, uint8_t *dest, uint32_t length) const;
    bool copyBytes(ByteOffset byteOffset, uint8_t *dest, uint32_t length) const;
    bool preloadBlock(ByteOffset byteOffset) const;
};

#endif
