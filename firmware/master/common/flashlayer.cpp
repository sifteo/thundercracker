
#include "flashlayer.h"
#include "flash.h"
#include <sifteo.h>

#include <string.h>


FlashLayer::CachedBlock FlashLayer::blocks[NUM_BLOCKS] BLOCK_ALIGN;
uint32_t FlashLayer::validBlocksMask = 0;
uint32_t FlashLayer::freeBlocksMask = 0;

#ifdef SIFTEO_SIMULATOR
FlashLayer::Stats FlashLayer::stats;
#endif

void FlashLayer::init()
{
    memset(blocks, 0, sizeof(blocks));
    // all blocks begin their lives free
    for (unsigned i = 0; i < NUM_BLOCKS; ++i) {
        Atomic::SetLZ(freeBlocksMask, i);
    }
}

bool FlashLayer::getRegion(unsigned offset, unsigned len, FlashRegion *r)
{
    CachedBlock *b = getCachedBlock(offset);
    if (b == 0) {
#ifdef SIFTEO_SIMULATOR
        stats.misses++;
#endif
        b = getFreeBlock();
        if (b == 0) {
            return false;
        }

        // find the start address of the block that contains this offset,
        // and read in the entire block.
        b->address = (offset / BLOCK_SIZE) * BLOCK_SIZE;
        Flash::read(b->address, (uint8_t*)b->data, BLOCK_SIZE);

        // mark it as both valid & no longer free
        unsigned idx = b - blocks;
        ASSERT(idx < NUM_BLOCKS);
        Atomic::SetLZ(validBlocksMask, idx);
        Atomic::ClearLZ(freeBlocksMask, idx);
    }
#ifdef SIFTEO_SIMULATOR
    else {
        stats.hits++;
    }

    if (stats.hits + stats.misses >= 1000) {
        DEBUG_LOG(("Flashlayer, stats for last 1000 accesses: %u hits, %u misses\n", stats.hits, stats.misses));
        stats.hits = stats.misses = 0;
    }
#endif

    if (offset + len > b->address + sizeof(b->data)) {
        // requested region crosses block boundary :(
        r->_size = b->address + BLOCK_SIZE - offset;
    }
    else {
        r->_size = len;
    }

    r->_address = offset;
    r->_data = b->data + (offset % BLOCK_SIZE);
    return true;
}

void FlashLayer::releaseRegion(const FlashRegion &r) {
    if (CachedBlock *b = getCachedBlock(r._address)) {
        unsigned idx = b - blocks;
        ASSERT(idx < NUM_BLOCKS);
        Atomic::SetLZ(freeBlocksMask, idx);
    }
}

FlashLayer::CachedBlock* FlashLayer::getCachedBlock(uintptr_t address) {
    uint32_t mask = validBlocksMask;
    while (mask) {
        unsigned idx = Intrinsic::CLZ(mask);
        CachedBlock *b = &blocks[idx];
        if (address >= b->address && address < b->address + BLOCK_SIZE) {
            return b;
        }
        Atomic::ClearLZ(mask, idx);
    }
    return 0;
}

FlashLayer::CachedBlock* FlashLayer::getFreeBlock() {
    if (freeBlocksMask == 0) {
        return 0;
    }

    unsigned idx = Intrinsic::CLZ(freeBlocksMask);
    return &blocks[idx];
}
