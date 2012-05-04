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

    // We *do* want ByteOffset to be 32-bit. It is important that we discard
    // extraneous high-bits on 64-bit hosts when translating flash addresses.

    typedef uint32_t ByteOffset;    // Number of bytes from the beginning of the span
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
        FlashAllocSpan result = { map, firstBlock, numBlocks };
        ASSERT(firstBlock < FlashAllocMap::NUM_CACHE_BLOCKS);
        ASSERT(numBlocks < FlashAllocMap::NUM_CACHE_BLOCKS);
        ASSERT(result.firstBlock == firstBlock);
        ASSERT(result.numBlocks == numBlocks);
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

    uint32_t sizeInBytes() const {
        return (uint32_t)numBlocks * FlashBlock::BLOCK_SIZE;
    }

    uint32_t firstByte() const {
        return (uint32_t)firstBlock * FlashBlock::BLOCK_SIZE;
    }

    bool offsetIsValid(ByteOffset byteOffset) const {
        return byteOffset < sizeInBytes();
    }

    /// Split off a portion of this FlashAllocSpan as a new span.
    FlashAllocSpan split(unsigned blockOffset,
        unsigned blockCount = FlashAllocMap::NUM_CACHE_BLOCKS) const;

    /// Range specified in bytes. Offset must be aligned, length is rounded up.
    FlashAllocSpan splitRoundingUp(unsigned byteOffset, unsigned byteCount) const;

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


/**
 * Header for a particular allocation object in flash. Every FlashAllocBlock
 * falls into one of three categories:
 *
 *   1. It contains its own valid FlashAllocHeader.
 *   2. It is referenced by a preceeding valid FlashAllocHeader.
 *   3. It is unallocated.
 *
 * Large objects that occupy more than one FlashAllocBlock only need a single
 * FlashAllocHeader, on the object's first block. This allows a single
 * FlashAllocSpan to reference the entirety of the object's data.
 *
 * In order to provide robustness against flash failures as well as robustness
 * against power outages that occur between erasing an AllocBlock and fully
 * writing its header, we keep a CRC for the header. If the CRC is not valid,
 * the AllocBlock is treated as unallocated.
 *
 * For wear leveling, an AllocHeader must also store erase counts for every
 * block it's referencing. These erase counts are 32-bit monotonically
 * increasing integers, incremented every time the specified AllocBlock
 * is erased.
 *
 * XXX: Work in progress.
 */

struct FlashAllocHeader
{
    static const unsigned NUM_BLOCKS = 4;
    static const unsigned NUM_BYTES = FlashBlock::BLOCK_SIZE * NUM_BLOCKS;
    static const unsigned NUM_WORDS = NUM_BYTES / sizeof(uint32_t);
    static const uint32_t MAGIC = 0x74666953;   // 'Sift'

    enum Type {
        T_OBJECT = 0x4a424f46,  // 'FOBJ' = large object
    };

    // Universal header
    struct {
        uint32_t magic;
        uint32_t type;
        uint32_t crc;
    } h;

    // Type-specific data occupies the rest of the block
    union {
        uint32_t words[NUM_WORDS - 3];
        struct {
            FlashAllocMap map;
            uint32_t eraseCounts[FlashAllocMap::NUM_ALLOC_BLOCKS];
        } object;
    } u;
};


#endif
