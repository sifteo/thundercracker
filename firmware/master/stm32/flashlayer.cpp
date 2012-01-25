
#include "flashlayer.h"
#include "macronixmx25.h"
#include "board.h"
#include <string.h>

FlashLayer::CachedBlock FlashLayer::blocks[NUM_BLOCKS];
uint32_t FlashLayer::validBlocksMask = 0;
uint32_t FlashLayer::freeBlocksMask = 0;

static MacronixMX25 &flash = MacronixMX25::instance;

void FlashLayer::init()
{
    memset(blocks, 0, sizeof(blocks));
    // all blocks begin their lives free
    for (unsigned i = 0; i < NUM_BLOCKS; ++i) {
        Atomic::SetLZ(freeBlocksMask, i);
    }
}

void* FlashLayer::getRegionFromOffset(int offset, int len, int *size)
{
    CachedBlock *b = getCachedBlock(offset);
    if (b == 0) {
        b = getFreeBlock();
        if (b == 0) {
            return 0;
        }

        // find the start address of the block that contains this offset,
        // and read in the entire block.
        b->address = (offset / BLOCK_SIZE) * BLOCK_SIZE;
        flash.read(b->address, (uint8_t*)b->data, BLOCK_SIZE);

        // mark it as both valid & no longer free
        unsigned idx = b - blocks;
        ASSERT(idx < NUM_BLOCKS);
        Atomic::SetLZ(validBlocksMask, idx);
        Atomic::ClearLZ(freeBlocksMask, idx);
    }

    int boff = offset % BLOCK_SIZE;
    *size = len;

    if ((unsigned)offset + len > b->address + sizeof(b->data)) {
        // requested region crosses block boundary :(
        *size = b->address + BLOCK_SIZE - offset;
    }

    return b->data + boff;
}
