/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_blockcache.h"
#include "flash_device.h"
#include "svmdebugger.h"
#include "panic.h"
#include <string.h>

uint8_t FlashBlock::mem[NUM_CACHE_BLOCKS][BLOCK_SIZE] BLOCK_ALIGN;
FlashBlock FlashBlock::instances[NUM_CACHE_BLOCKS];
uint16_t FlashBlock::validCodeBytes[NUM_CACHE_BLOCKS];
unsigned FlashBlock::latestStamp;


void FlashBlock::init()
{
    // All blocks start out with no valid data
    for (unsigned i = 0; i < NUM_CACHE_BLOCKS; ++i) {
        instances[i].address = INVALID_ADDRESS;

        // We explicitly store the ID of each block,
        // so that id() and getData() can be as fast as possible.
        instances[i].idByte = i;
    }

    FLASHLAYER_STATS_ONLY(resetStats());
}

void FlashBlock::get(FlashBlockRef &ref, uint32_t blockAddr, unsigned flags)
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

        FlashBlock *recycled = recycleBlock(blockAddr);
        ASSERT(recycled->refCount == 0);
        ASSERT(recycled >= &instances[0] && recycled < &instances[NUM_CACHE_BLOCKS]);

        recycled->load(blockAddr, flags);
        ref.set(recycled);
    }
    
    // Update this block's access stamp (See recycleBlock)
    ref->stamp = ++latestStamp;

    FLASHLAYER_STATS_ONLY(stats.periodic.blockTotal++);
    FLASHLAYER_STATS_ONLY(dumpStats());
}

void FlashBlock::anonymous(FlashBlockRef &ref)
{
    /*
     * Get a new anonymous block. This block starts out with an invalid address,
     * and a single ref (owned by the caller).
     *
     * The initial data contents of the block are undefined. It may contain
     * sensitive information, for example, that should not be allowed to leak
     * to userspace.
     *
     * The block may be used as-is in order to borrow memory from the cache
     * temporarily, or it may be written to a physical location using
     * FlashBlockWriter::commitBlock(). The latter use case supports using
     * the cache as a place to prepare pages for later writing, even when
     * the write address is not known ahead-of-time.
     */

    FlashBlock *recycled = recycleBlock(INVALID_ADDRESS);
    ASSERT(recycled->refCount == 0);
    ASSERT(recycled >= &instances[0] && recycled < &instances[NUM_CACHE_BLOCKS]);

    // This ensures nobody else will ref the same block.
    recycled->address = INVALID_ADDRESS;
    recycled->validCodeBytes[recycled->id()] = 0;

    ref.set(recycled);
    ASSERT(recycled->refCount == 1);
}

void FlashBlock::anonymous(FlashBlockRef &ref, uint8_t fillByte)
{
    /*
     * Get a new anonymous block, and fill it with "fillByte"
     */

    anonymous(ref);
    memset(ref->getData(), fillByte, BLOCK_SIZE);
}

ALWAYS_INLINE FlashBlock *FlashBlock::lookupBlock(uint32_t blockAddr)
{
    /*
     * This is a compromise between a fully associative and a set-associative
     * cache. We can't usefully be set-associative with a small N, because
     * our N would need to be at least as large as the worst-case number of
     * referenced blocks.
     *
     * So instead, we'll still be a fully associative cache, but we use
     * the address as a hint for which cache bucket to start in. We strongly
     * prefer to place blocks in this cached address, but we don't require
     * it (in case the preferred location is referenced or very recently
     * accessed).
     */

    ASSERT((blockAddr & BLOCK_MASK) == 0);
    FlashBlock *ptr = &instances[(blockAddr >> BLOCK_SIZE_LOG2) % NUM_CACHE_BLOCKS];
    unsigned count = NUM_CACHE_BLOCKS;

    do {
        if (ptr->address == blockAddr)
            return ptr;
        if (++ptr == &instances[NUM_CACHE_BLOCKS])
            ptr = &instances[0];
    } while (--count);

    return 0;
}

FlashBlock *FlashBlock::recycleBlock(uint32_t blockAddr)
{
    /*
     * Look for a block we can recycle, in order to service a cache miss.
     * To maintain the hash property of our cache, we strongly prefer to
     * recycle the block which is directly mapped to the requested address.
     *
     * We opt to skip to the next block if the preferred block is referenced,
     * or if it was used recently. (Its stamp is recent).
     */

    // First pass: Look for something both unreferenced and stale
    {
        FlashBlock *ptr = &instances[(blockAddr >> BLOCK_SIZE_LOG2) % NUM_CACHE_BLOCKS];
        unsigned count = NUM_CACHE_BLOCKS;
        const unsigned ageThreshold = NUM_CACHE_BLOCKS * 2;
        unsigned localLatestStamp = latestStamp;

        do {
            if (ptr->refCount == 0 && (ptr->address == INVALID_ADDRESS
                    || ptr->getAge(localLatestStamp) >= ageThreshold))
                return ptr;
            if (++ptr == &instances[NUM_CACHE_BLOCKS])
                ptr = &instances[0];
        } while (--count);
    }

    // Second pass: Give up on finding a stale block, just look for anything unreferenced
    {
        FlashBlock *ptr = &instances[(blockAddr >> BLOCK_SIZE_LOG2) % NUM_CACHE_BLOCKS];
        unsigned count = NUM_CACHE_BLOCKS;

        do {
            if (ptr->refCount == 0)
                return ptr;
            if (++ptr == &instances[NUM_CACHE_BLOCKS])
                ptr = &instances[0];
        } while (--count);
    }

    ASSERT(0 && "Oh no, all cache blocks are in use. Is there a reference leak?");
    PanicMessenger::haltForever();
    return 0;
}

void FlashBlock::load(uint32_t blockAddr, unsigned flags)
{
    /*
     * Handle a cache miss or invalidation. Load this block with new data
     * from blockAddr.
     */

    ASSERT(blockAddr != INVALID_ADDRESS);
    ASSERT((blockAddr & (BLOCK_SIZE - 1)) == 0);

    validCodeBytes[id()] = 0;
    address = blockAddr;

    uint8_t *data = getData();
    ASSERT(isAddrValid(reinterpret_cast<uintptr_t>(data)));
    ASSERT(isAddrValid(reinterpret_cast<uintptr_t>(data + BLOCK_SIZE - 1)));

    if (flags & F_KNOWN_ERASED) {
        // This is an important optimization which prevents us from reading
        // blocks that we've just erased, especially while writing to a new volume.
        memset(data, 0xFF, BLOCK_SIZE);
        DEBUG_ONLY(verify());

    } else {
        // Normal cache miss; fetch from hardware
        FlashDevice::read(blockAddr, data, BLOCK_SIZE);
        FLASHLAYER_STATS_ONLY(countBlockMiss(blockAddr));
    }

    SvmDebugger::patchFlashBlock(blockAddr, data);
}

void FlashBlockWriter::beginBlock(uint32_t blockAddr)
{
    if (ref.isHeld() && ref->getAddress() == blockAddr) {
        // Nothing to do, already writing to the same block
        return;
    }

    FlashBlockRef newRef;
    FlashBlock::get(newRef, blockAddr);
    beginBlock(newRef);
}

void FlashBlockWriter::beginBlock(const FlashBlockRef &r)
{
    ASSERT(r.isHeld());
    if (ref.isHeld() && ref->getAddress() == r->getAddress()) {
        // Nothing to do, already writing to the same block
        return;
    }

    // Switching blocks
    commitBlock();
    ref = r;
    ASSERT(ref.isHeld());

    // Prepare to write
    ref->validCodeBytes[ref->id()] = 0;
}

void FlashBlockWriter::beginBlock()
{
    /*
     * Begin a new anonymous block. A specific address must be specified
     * via relocate().
     */

    commitBlock();
    FlashBlock::anonymous(ref, 0xFF);
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

    invalidate(0, 0xFFFFFFFF);
}

void FlashBlock::invalidate(uint32_t addrBegin, uint32_t addrEnd)
{
    /*
     * Invalidate any cache blocks which overlap the given range of
     * flash addresses.
     */

    ASSERT(addrBegin < addrEnd);

    for (unsigned idx = 0; idx < NUM_CACHE_BLOCKS; idx++) {
        FlashBlock *block = &instances[idx];
        uint32_t blockAddrBegin = block->address;
        uint32_t blockAddrEnd = blockAddrBegin + FlashBlock::BLOCK_SIZE;

        if (blockAddrBegin == INVALID_ADDRESS)
            continue;

        if (blockAddrEnd > addrBegin && blockAddrBegin < addrEnd) {
            // Block is in range. Invalidate it.

            if (block->refCount) {
                // In use. If it's backed by flash at all, reload it.
                if (block->address != INVALID_ADDRESS)
                    block->load(block->address);
            } else {
                // Nobody's using this block, quietly mark it as invalid / anonymous
                block->address = INVALID_ADDRESS;
            }
        }
    }
}

void FlashBlockWriter::commitBlock()
{
    /*
     * Write a modified flash block back to the device.
     */

    if (ref.isHeld()) {
        FlashBlock *block = &*ref;

        // Must not have tried to run code from this block during a write.
        ASSERT(ref->validCodeBytes[ref->id()] == 0);

        // Must not be anonymous
        ASSERT(block->address != FlashBlock::INVALID_ADDRESS);

        FlashDevice::write(block->address, block->getData(),
            FlashBlock::BLOCK_SIZE);

        // Make sure we are only programming bits from 1 to 0.
        DEBUG_ONLY(block->verify());

        ref.release();
    }
}

void FlashBlockWriter::relocate(uint32_t blockAddr)
{
    /*
     * Write this flash block to a new address in the next commitBlock().
     *
     * If the block was anonymous, this has the effect of committing it to
     * a physical address. If the block already had a physical address,
     * this copies it to an additional location. In either case, the block's
     * "address" will change to match the new blockAddr.
     *
     * You must be the sole owner of this FlashBlock, and no references
     * may be held to the destination block. (This provision is required
     * in order to avoid having two FlashBlockRef holders use different
     * RAM addresses for the same flash address. It would break cache
     * coherency.)
     */

    ASSERT((blockAddr & FlashBlock::BLOCK_MASK) == 0);

    // Unlike commitBlock(), we must have exactly one reference. Always.
    ASSERT(ref.isHeld());
    FlashBlock *block = &*ref;
    ASSERT(block->refCount == 1);

    // Same as commitBlock() if we aren't moving.
    if (block->address != blockAddr) {

        // Invalidate any blocks we're replacing. They must be unref'ed.
        for (unsigned idx = 0; idx < FlashBlock::NUM_CACHE_BLOCKS; idx++) {
            FlashBlock *b = &FlashBlock::instances[idx];
            if (b->address == blockAddr) {

                if (b->refCount != 0) {
                    LOG(("FLASH: Serious Error! Detected an attempt to relocate "
                        "anonymous block over referenced block. Did someone "
                        "delete a volume which still had outstanding references?\n"));
                    ASSERT(0);
                }

                b->address = FlashBlock::INVALID_ADDRESS;
            }
        }

        // Replace this block's address in the cache.
        block->address = blockAddr;
    }
}

void FlashBlock::cacheEraseSector(uint32_t sectorAddr)
{
    /*
     * A lighter-weight alternative to invalidate(), used when erasing one
     * flash sector. Any cached blocks in this sector which have a reference
     * are erased, and any unreferenced blocks are evicted.
     *
     * Cached blocks not part of this sector are left alone.
     */

    const unsigned mask = ~(FlashDevice::SECTOR_SIZE - 1);
    ASSERT((sectorAddr & mask) == sectorAddr);

    for (unsigned idx = 0; idx < NUM_CACHE_BLOCKS; idx++) {
        FlashBlock *block = &instances[idx];

        if ((block->address & mask) == sectorAddr) {
            if (block->refCount)
                block->load(block->address, F_KNOWN_ERASED);
            else
                block->address = INVALID_ADDRESS;
        }
    }
}

void FlashBlock::preload(uint32_t blockAddr)
{
    // XXX: Implement me
}
