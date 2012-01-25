#include <stdlib.h>
#include "flashlayer.h"
#include <sifteo/macros.h>

// statics
FlashLayer::CachedBlock FlashLayer::blocks[NUM_BLOCKS];
struct FlashLayer::Stats FlashLayer::stats;
uint32_t FlashLayer::validBlocksMask = 0;
uint32_t FlashLayer::freeBlocksMask = 0;

void *FlashLayer::getRegionFromOffset(int offset, int len, int *size)
{
    CachedBlock *b = getCachedBlock(offset);
    if (b == 0) {
        stats.misses++;
        b = getFreeBlock();
        if (b == 0) {
            LOG(("Flashlayer: No free blocks in cache\n"));
            return 0;
        }
        
        int bpos = offset / BLOCK_SIZE;
        ASSERT(fseek(mFile, bpos * BLOCK_SIZE, SEEK_SET) == 0);
        
        size_t result = fread (b->data, 1, BLOCK_SIZE, mFile);
        if (result != (size_t)BLOCK_SIZE) {
            // TODO: Should we pad asset files to 512?  If not, zero out the remainder of this block.
            *size = result;
        }
        
        b->address = bpos * BLOCK_SIZE;

        // mark it as both valid & no longer free
        unsigned idx = b - blocks;
        ASSERT(idx < NUM_BLOCKS);
        Atomic::SetLZ(validBlocksMask, idx);
        Atomic::ClearLZ(freeBlocksMask, idx);
    }
    else {
        stats.hits++;
    }

    if (stats.hits + stats.misses >= 1000) {
        DEBUG_LOG(("Flashlayer, stats for last 1000 accesses: %u hits, %u misses\n", stats.hits, stats.misses));
        stats.hits = stats.misses = 0;
    }
    
    int boff = offset % BLOCK_SIZE;
    *size = len;
    
    if ((unsigned)offset + len > b->address + sizeof(b->data)) {
        // requested region crosses block boundary :(
        int valid = b->address + BLOCK_SIZE - offset;
        *size = valid;
    }
    
    return b->data + boff;
}

// File hacks

FILE * FlashLayer::mFile = NULL;

void FlashLayer::init()
{
    // all blocks begin their lives free
    for (unsigned i = 0; i < NUM_BLOCKS; ++i) {
        Atomic::SetLZ(freeBlocksMask, i);
    }
    mFile = fopen("asegment.bin", "rb");
    ASSERT(mFile != NULL && "ERROR: FlashLayer failed to open asset segment - make sure asegment.bin is next to your executable.\n");
    stats.hits = stats.misses = 0;
}
