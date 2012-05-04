/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_blockcache.h"
#include "flash_device.h"
#include "svmdebugger.h"
#include <string.h>

uint8_t FlashBlock::mem[NUM_CACHE_BLOCKS][BLOCK_SIZE] BLOCK_ALIGN;
FlashBlock FlashBlock::instances[NUM_CACHE_BLOCKS];
uint32_t FlashBlock::referencedBlocksMap;
uint32_t FlashBlock::latestStamp;


void FlashBlock::init()
{
    // All blocks start out with no valid data
    for (unsigned i = 0; i < NUM_CACHE_BLOCKS; ++i) {
        instances[i].address = INVALID_ADDRESS;
    }

    FLASHLAYER_STATS_ONLY(resetStats());
}

void FlashBlock::get(FlashBlockRef &ref, uint32_t blockAddr)
{
    ASSERT((blockAddr & BLOCK_MASK) == 0);

    if (ref.isHeld() && ref->address == blockAddr) {
        // Cache layer 1: Repeated access to the same block. Keep existing ref.
        FLASHLAYER_STATS_ONLY(stats.periodic.blockHitSame++);

    } else if (FlashBlock *cached = lookupBlock(blockAddr)) {
        // Cache layer 2: Block exists elsewhere in the cache
        FLASHLAYER_STATS_ONLY(stats.periodic.blockHitOther++);
        ref.set(cached);

    } else {
        // Cache miss. Find a free block and reload it. Reset the lazy
        // code validator.

        FlashBlock *recycled = recycleBlock();
        ASSERT(recycled->refCount == 0);
        ASSERT(recycled >= &instances[0] && recycled < &instances[NUM_CACHE_BLOCKS]);
        FLASHLAYER_STATS_ONLY(countBlockMiss(blockAddr));

        recycled->load(blockAddr);
        ref.set(recycled);
    }
    
    // Update this block's access stamp (See recycleBlock)
    ref->stamp = ++latestStamp;

    FLASHLAYER_STATS_ONLY(stats.periodic.blockTotal++);
    FLASHLAYER_STATS_ONLY(dumpStats());
}

FlashBlock *FlashBlock::lookupBlock(uint32_t blockAddr)
{
    // Any slot in the cache may have a valid block, even if
    // the refcount is zero. Right now there's no faster way to find
    // a block than performing a linear search.
    
    for (unsigned i = 0; i < NUM_CACHE_BLOCKS; ++i) {
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
    // blocks that are zero in referencedBlocksMap.
    //
    // Of these blocks, we look for the one that's most stale. We have a simple
    // 32-bit timestamp which is updated every time a cache hit occurs. We
    // can quickly scan through and find the block with the oldest stamp.

    STATIC_ASSERT(NUM_CACHE_BLOCKS <= 32);
    const uint32_t allBlocks = ((1 << NUM_CACHE_BLOCKS) - 1) << (32 - NUM_CACHE_BLOCKS);
    uint32_t availableBlocks = allBlocks & ~referencedBlocksMap;
    ASSERT(availableBlocks &&
        "Oh no, all cache blocks are in use. Is there a reference leak?");

    FlashBlock *bestBlock = 0;
    uint32_t bestAge = 0;

    do {
        unsigned idx = Intrinsic::CLZ(availableBlocks);
        FlashBlock *block = &instances[idx];
        uint32_t age = latestStamp - block->stamp;  // Wraparound-safe

        DEBUG_ONLY({
            /*
             * While we're here, make sure that any available blocks are actually
             * clean. If we're writing a block, it must be referenced, and we must
             * commit that write afterwards.
             */
            if (block->address != INVALID_ADDRESS)
                block->verify();
        })

        if (age >= bestAge) {
            bestBlock = block;
            bestAge = age;
        }
        
        availableBlocks &= ~Intrinsic::LZ(idx);
    } while (availableBlocks);

    return bestBlock;
}

void FlashBlock::load(uint32_t blockAddr)
{
    /*
     * Handle a cache miss or invalidation. Load this block with new data
     * from blockAddr.
     */

    ASSERT((blockAddr & (BLOCK_SIZE - 1)) == 0);
    validCodeBytes = 0;
    address = blockAddr;

    uint8_t *data = getData();
    ASSERT(isAddrValid(reinterpret_cast<uintptr_t>(data)));
    ASSERT(isAddrValid(reinterpret_cast<uintptr_t>(data + BLOCK_SIZE - 1)));

    FlashDevice::read(blockAddr, data, BLOCK_SIZE);
    SvmDebugger::patchFlashBlock(blockAddr, data);
}

void FlashBlockWriter::commit()
{
    /*
     * Write a modified flash block back to the device.
     */

    FlashBlock *block = &*ref;

    // Must not have tried to run code from this block during a write.
    ASSERT(ref->validCodeBytes == 0);

    FlashDevice::write(block->address, block->getData(),
        FlashBlock::BLOCK_SIZE);

    // Make sure we are only programming bits from 1 to 0.
    DEBUG_ONLY(block->verify());
}

void FlashBlock::invalidate()
{
    /*
     * Invalidate the whole cache. This is a pretty heavyweight operation
     * clearly, and we plan to only do this for debugging, or when writing
     * to flash.
     *
     * Any blocks with no reference are simply evicted from the cache without
     * replacement. Blocks with a reference are reloaded in-place.
     */

    for (unsigned idx = 0; idx < NUM_CACHE_BLOCKS; idx++) {
        FlashBlock *block = &instances[idx];

        if (block->refCount)
            block->load(block->address);
        else
            block->address = INVALID_ADDRESS;
    }
}

void FlashBlock::preload(uint32_t blockAddr)
{
    // XXX: Implement me
}
