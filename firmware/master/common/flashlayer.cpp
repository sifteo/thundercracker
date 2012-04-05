/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flashlayer.h"
#include "flash.h"
#include "svmdebugpipe.h"
#include "svmmemory.h"
#include "svmdebugger.h"
#include <string.h>

#ifdef SIFTEO_SIMULATOR
struct FlashStats gFlashStats;
#endif

uint8_t FlashBlock::mem[NUM_BLOCKS][BLOCK_SIZE] BLOCK_ALIGN;
FlashBlock FlashBlock::instances[NUM_BLOCKS];
uint32_t FlashBlock::referencedBlocksMap;
uint32_t FlashBlock::latestStamp;


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
        FLASHLAYER_STATS_ONLY(gFlashStats.blockHitSame++);

    } else if (FlashBlock *cached = lookupBlock(blockAddr)) {
        // Cache layer 2: Block exists elsewhere in the cache
        FLASHLAYER_STATS_ONLY(gFlashStats.blockHitOther++);
        ref.set(cached);

    } else {
        // Cache miss. Find a free block and reload it. Reset the lazy
        // code validator.

        FlashBlock *recycled = recycleBlock();
        ASSERT(recycled->refCount == 0);
        ASSERT(recycled >= &instances[0] && recycled < &instances[NUM_BLOCKS]);
        FLASHLAYER_STATS_ONLY(gFlashStats.blockMiss++);

        recycled->load(blockAddr);
        ref.set(recycled);
    }
    
    // Update this block's access stamp (See recycleBlock)
    ref->stamp = ++latestStamp;

#ifdef SIFTEO_SIMULATOR
    if (gFlashStats.enabled && ++gFlashStats.blockTotal >= 1000) {
        SysTime::Ticks now = SysTime::ticks();
        SysTime::Ticks tickDiff = now - gFlashStats.timestamp;
        float dt = tickDiff / (float) SysTime::sTicks(1);

        if (dt > 1.0f) {
            gFlashStats.timestamp = now;
        
            uint32_t totalBytes = gFlashStats.streamBytes
                + gFlashStats.blockMiss * BLOCK_SIZE;

            const float flashBusMHZ = 18.0f;
            const float bytesToMBits = 10.0f * 1e-6;
            float effectiveMHZ = totalBytes / dt * bytesToMBits;

            LOG(("Flashlayer: %9.1f acc/s, %8.1f same/s, "
                "%8.1f cached/s, %8.1f miss/s, %5.1f stream kB/s, "
                "%8.2f%% bus utilization\n",
                gFlashStats.blockTotal / dt,
                gFlashStats.blockHitSame / dt,
                gFlashStats.blockHitOther / dt,
                gFlashStats.blockMiss / dt,
                gFlashStats.streamBytes / dt * 1e-3,
                effectiveMHZ / flashBusMHZ * 100.0f));

            gFlashStats.blockTotal = 0;
            gFlashStats.blockHitSame = 0;
            gFlashStats.blockHitOther = 0;
            gFlashStats.blockMiss = 0;
            gFlashStats.streamBytes = 0;

            for (unsigned i = 0; i < NUM_BLOCKS; i++) {
                FlashBlock &block = instances[i];
                SvmMemory::VirtAddr va = SvmMemory::flashToVirtAddr(block.address);
                std::string name = SvmDebugPipe::formatAddress(va);

                LOG(("\tblock %02d: addr=0x%06x stamp=0x%08x cb=0x%03x ref=%d va=%08x  %s\n",
                    i, block.address, block.stamp, block.validCodeBytes,
                    block.refCount, (unsigned)va, name.c_str()));
            }
        }
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
    // blocks that are zero in referencedBlocksMap.
    //
    // Of these blocks, we look for the one that's most stale. We have a simple
    // 32-bit timestamp which is updated every time a cache hit occurs. We
    // can quickly scan through and find the block with the oldest stamp.

    STATIC_ASSERT(NUM_BLOCKS <= 32);
    const uint32_t allBlocks = ((1 << NUM_BLOCKS) - 1) << (32 - NUM_BLOCKS);
    uint32_t availableBlocks = allBlocks & ~referencedBlocksMap;
    ASSERT(availableBlocks &&
        "Oh no, all cache blocks are in use. Is there a reference leak?");

    FlashBlock *bestBlock = 0;
    uint32_t bestAge = 0;

    do {
        unsigned idx = Intrinsic::CLZ(availableBlocks);
        FlashBlock *block = &instances[idx];
        uint32_t age = latestStamp - block->stamp;  // Wraparound-safe

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
     * from bockAddr.
     */

    ASSERT((blockAddr & (BLOCK_SIZE - 1)) == 0);
    validCodeBytes = 0;
    address = blockAddr;

    uint8_t *data = getData();
    ASSERT(isAddrValid(reinterpret_cast<uintptr_t>(data)));
    ASSERT(isAddrValid(reinterpret_cast<uintptr_t>(data + BLOCK_SIZE - 1)));

    Flash::read(blockAddr, data, BLOCK_SIZE);
    SvmDebugger::patchFlashBlock(blockAddr, data);
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

    for (unsigned idx = 0; idx < NUM_BLOCKS; idx++) {
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

uint32_t FlashStream::read(uint8_t *dest, uint32_t maxLength)
{
    uint32_t chunk = MIN(maxLength, remaining());
    if (chunk)
        Flash::read(getAddress() + offset, dest, chunk);
    FLASHLAYER_STATS_ONLY(gFlashStats.streamBytes += chunk);
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
