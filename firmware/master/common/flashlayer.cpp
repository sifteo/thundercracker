/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flashlayer.h"
#include "flash.h"
#include <sifteo.h>

#include <string.h>


uint8_t FlashBlock::mem[NUM_BLOCKS][BLOCK_SIZE] BLOCK_ALIGN;
FlashBlock FlashBlock::instances[NUM_BLOCKS];
uint32_t FlashBlock::referencedBlocksMap;
uint32_t FlashBlock::busyBlocksMap;
FlashBlock::Stats FlashBlock::stats;


void FlashBlock::init()
{
    // All blocks start out with no valid data
    for (unsigned i = 0; i < NUM_BLOCKS; ++i) {
        instances[i].address = INVALID_ADDRESS;
    }
}

void FlashBlock::get(FlashBlockRef &ref, uint32_t blockAddr)
{
    ASSERT((blockAddr & BLOCK_MASK) == 0);

    if (ref.isHeld() && ref->address == blockAddr) {
        // Cache layer 1: Repeated access to the same block. Keep existing ref.
        FLASHLAYER_STATS_ONLY(stats.hit_same++);

    } else if (FlashBlock *cached = lookupBlock(blockAddr)) {
        // Cache layer 2: Block exists elsewhere in the cache
        FLASHLAYER_STATS_ONLY(stats.hit_other++);
        ref.set(cached);

    } else {
        // Cache miss. Find a free block and reload it. Reset the lazy
        // code validator.

        FlashBlock *recycled = recycleBlock();
        ASSERT(recycled->refCount == 0);
        ASSERT(recycled >= &instances[0] && recycled < &instances[NUM_BLOCKS]);
        FLASHLAYER_STATS_ONLY(stats.miss++);

        recycled->validCodeBytes = 0;
        recycled->address = blockAddr;

        uint8_t *data = recycled->getData();
        ASSERT(isAddrValid(reinterpret_cast<uintptr_t>(data)));
        ASSERT(isAddrValid(reinterpret_cast<uintptr_t>(data + BLOCK_SIZE - 1)));

        Flash::read(blockAddr, data, BLOCK_SIZE);
        ref.set(recycled);
    }
    
    // Mark this block as busy. This is a hint used by recycleBlock().
    busyBlocksMap |= ref->bit();

#ifdef FLASHLAYER_STATS
    if (++stats.total >= 10000) {
        LOG(("Flashlayer, stats for last %u accesses: "
            "%6u same-block hits, %6u other-block hits, %6u misses, %2u refs\n",
            stats.total, stats.hit_same, stats.hit_other, stats.miss,
            stats.global_refcount));

        stats.total = 0;
        stats.hit_same = 0;
        stats.hit_other = 0;
        stats.miss = 0;
    }
#endif
}

uint8_t *FlashBlock::getByte(FlashBlockRef &ref, uint32_t address)
{
    uint32_t offset = address & BLOCK_MASK;
    get(ref, address & ~BLOCK_MASK);
    return ref->getData() + offset;
}

uint8_t *FlashBlock::getBytes(FlashBlockRef &ref, uint32_t address, uint32_t &length)
{
    uint32_t offset = address & BLOCK_MASK;
    uint32_t maxLength = BLOCK_SIZE - offset;
    if (length > maxLength)
        length = maxLength;

    return getByte(ref, address);
}   

FlashBlock *FlashBlock::lookupBlock(uint32_t blockAddr)
{
    // Any slot in the cache may have a valid block, even if
    // the refcount is zero. Right now there's no faster way to find
    // a block than performing a linear search.
    
    for (unsigned i = 0; i < NUM_BLOCKS; ++i) {
        FlashBlock *b = &instances[i];
        if (b->address == blockAddr)
            return b;
    }
    return NULL;
}

FlashBlock *FlashBlock::recycleBlock()
{
    // Look for a block we can recycle, in order to service a cache miss.
    // This block *must* have a refcount of zero, so we only search the
    // blocks that are zero in referencedBlocksMap. Additionally, we prefer
    // to look at blocks that haven't been used since the last cache miss.
    //
    // XXX: This algorithm doesn't actually work in practice, the idleBlocks
    //      mask isn't inclusive enough and we end up thrashing a lot. Fix me!!

    STATIC_ASSERT(NUM_BLOCKS <= 32);
    const uint32_t allBlocks = ((1 << NUM_BLOCKS) - 1) << (32 - NUM_BLOCKS);
    uint32_t availableBlocks = allBlocks & ~referencedBlocksMap;
    uint32_t idleBlocks = availableBlocks & ~busyBlocksMap;
    busyBlocksMap = 0;

    if (idleBlocks)
        return &instances[Intrinsic::CLZ(idleBlocks)];

    if (availableBlocks)
        return &instances[Intrinsic::CLZ(availableBlocks)];

    // This is really bad!
    ASSERT(0 && "Oh no, all cache blocks are in use. Is there a reference leak?");
    return &instances[0];
}

void FlashBlock::preload(uint32_t blockAddr)
{
    // XXX: Implement me
}

uint32_t FlashStream::read(uint8_t *dest, uint32_t maxLength)
{
    uint32_t chunk = MIN(maxLength, remaining());
    if (chunk)
        Flash::read(getAddress() + offset, dest, chunk);
    return chunk;
}

FlashRange FlashRange::split(uint32_t sliceOffset, uint32_t sliceSize) const
{
    // Catch underflow
    if (sliceOffset >= size)
        return FlashRange(getAddress(), 0);

    // Truncate overflows
    uint32_t maxSliceSize = getSize() - sliceOffset;
    if (sliceSize > maxSliceSize)
        sliceSize = maxSliceSize;
    
    return FlashRange(getAddress() + sliceOffset, sliceSize);
}
