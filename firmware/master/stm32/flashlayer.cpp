
#include "flashlayer.h"
#include "macronixmx25.h"
#include "board.h"
#include <string.h>

FlashLayer::CachedBlock FlashLayer::blocks[NUM_BLOCKS];
static MacronixMX25 &flash = MacronixMX25::instance;

void FlashLayer::init()
{
    memset(blocks, 0, sizeof(blocks));
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

        b->inUse = true;
        b->valid = true;
    }

    int boff = offset % BLOCK_SIZE;
    *size = len;

    if ((unsigned)offset + len > b->address + sizeof(b->data)) {
        // requested region crosses block boundary :(
        *size = b->address + BLOCK_SIZE - offset;
    }

    return b->data + boff;
}
