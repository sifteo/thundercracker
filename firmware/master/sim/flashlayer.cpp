
#include "flashlayer.h"

FlashLayer::CachedBlock FlashLayer::blocks[NUM_BLOCKS];

char* FlashLayer::getRegion(uint32_t address, int len)
{
    (void)len;
    return (char*)address;
#if 0 // eventually
    CachedBlock *b = getCachedBlock(address);
    if (b != 0) {
        if (address + len > b->address + sizeof(b->data)) {
            // requested region crosses block boundary :(
            return 0;
        }
        return b->data + (address - b->address);
    }
    // XXX - pull from external flash via DMA
    return 0;
#endif
}

void FlashLayer::releaseRegion(uint32_t address)
{
    (void)address;
}

/*
 * Try to find an existing cached block for the given address.
 */
FlashLayer::CachedBlock* FlashLayer::getCachedBlock(uint32_t address)
{
    FlashLayer::CachedBlock *b;
    int i;
    for (i = 0, b = FlashLayer::blocks; i < NUM_BLOCKS; i++, b++) {
        if (address >= b->address && address < b->address + BLOCK_SIZE) {
            return b;
        }
    }
    return 0;
}
