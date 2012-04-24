/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_allocation.h"
#include <algorithm>


FlashAllocSpan FlashAllocSpan::split(unsigned blockOffset, unsigned blockCount) const
{
    if (blockOffset >= numBlocks)
        return empty();
    else
        return create(map, firstBlock + blockOffset,
            std::min(blockCount, numBlocks - blockOffset));
}

bool FlashAllocSpan::flashAddrToOffset(FlashAddr flashAddr, ByteOffset &byteOffset) const
{
    if (!map)
        return false;

    // Split the flash address
    uint32_t allocBlock = flashAddr / FlashAllocBlock::BLOCK_SIZE;
    uint32_t allocOffset = flashAddr & FlashAllocBlock::BLOCK_MASK;

    // We have no direct index for this, so do a linear search in our map
    for (unsigned i = 0; i < arraysize(map->blocks); i++)
        if (map->blocks[i].id == allocBlock) {
            ByteOffset result = i * FlashAllocBlock::BLOCK_SIZE + allocOffset;
            if (offsetIsValid(result)) {
                byteOffset = result;
                return true;
            }
            return false;
        }

    // No match
    return false;
}

bool FlashAllocSpan::offsetToFlashAddr(ByteOffset byteOffset, FlashAddr &flashAddr) const
{
    if (!map)
        return false;

    if (!offsetIsValid(byteOffset))
        return false;
    
    // Split the byte address
    

    return false;
}

bool FlashAllocSpan::getBlock(FlashBlockRef &ref, ByteOffset byteOffset) const
{
    return false;
}

bool FlashAllocSpan::getBytes(FlashBlockRef &ref, ByteOffset byteOffset, PhysAddr &ptr, uint32_t &length) const
{
    return false;
}

bool FlashAllocSpan::getByte(FlashBlockRef &ref, ByteOffset byteOffset, PhysAddr &ptr) const
{
    return false;
}

bool FlashAllocSpan::preloadBlock(ByteOffset byteOffset) const
{
    FlashAddr flashAddr;

    if (offsetToFlashAddr(byteOffset, flashAddr)) {
        FlashBlock::preload(flashAddr);
        return true;
    }

    return false;
}

bool FlashAllocSpan::copyBytes(FlashBlockRef &ref, ByteOffset byteOffset, uint8_t *dest, uint32_t length) const
{
    PhysAddr srcPA;

    while (length) {
        uint32_t chunk = length;
        if (!getBytes(ref, byteOffset, srcPA, length))
            return false;

        memcpy(dest, srcPA, chunk);
        dest += chunk;
        byteOffset += chunk;
        length -= chunk;
    }

    return true;
}

bool FlashAllocSpan::copyBytes(ByteOffset byteOffset, uint8_t *dest, uint32_t length) const
{
    FlashBlockRef ref;
    return copyBytes(ref, byteOffset, dest, length);
}
