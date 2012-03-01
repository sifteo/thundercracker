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
    assert((blockAddr & BLOCK_MASK) == 0);

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
        assert(recycled->refCount == 0);
        FLASHLAYER_STATS_ONLY(stats.miss++);

        recycled->validCodeBytes = 0;
        recycled->address = blockAddr;
        Flash::read(blockAddr, recycled->getData(), BLOCK_SIZE);
        ref.set(recycled);
    }
    
    // Mark this block as busy. This is a hint used by recycleBlock().
    busyBlocksMap |= ref->bit();

#ifdef FLASHLAYER_STATS
    if (++stats.total >= 1000) {
        DEBUG_LOG(("Flashlayer, stats for last %u accesses: "
            "%u same-block hits, %u other-block hits, %u misses, %u refs\n",
            stats.hit_same, stats.hit_other, stats.miss,
            stats.global_refcount));

        stats.total = 0;
        stats.hit_same = 0;
        stats.hit_other = 0;
        stats.miss = 0;
    }
#endif
}

uint8_t *FlashBlock::getBytes(FlashBlockRef &ref, uint32_t address, uint32_t &length)
{
    uint32_t offset = address & BLOCK_MASK;
    uint32_t maxLength = BLOCK_SIZE - offset;

    if (length > maxLength)
        length = maxLength;

    get(ref, address - offset);
    return ref->getData() + offset;
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

FlashBlock *FlashBlock::recycleBlock(uint32_t blockAddr)
{
    // Look for a block we can recycle, in order to service a cache miss.
    // This block *must* have a refcount of zero, so we only search the
    // blocks that are zero in referencedBlocksMap. Additionally, we prefer
    // to look at blocks that haven't been used since the last cache miss.

    uint32_t availableBlocks = ~referencedBlocksMap;
    uint32_t idleBlocks = availableBlocks & ~busyBlocksMap;
    busyBlocksMap = 0;

    if (idleBlocks)
        return &instances[Intrinsic::CLZ(idleBlocks)];

    assert(availableBlocks && "Oh no, all cache blocks are in use. Is there a reference leak?");
    return &instances[Intrinsic::CLZ(availableBlocks)];
}
