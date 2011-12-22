
#include "flashlayer.h"

FlashLayer::CachedBlock FlashLayer::blocks[NUM_BLOCKS];

char* FlashLayer::getRegion(uintptr_t address, int len)
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

void FlashLayer::releaseRegion(uintptr_t address)
{
    (void)address;
}
