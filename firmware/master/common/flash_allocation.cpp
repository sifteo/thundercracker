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

    return create(map, firstBlock + blockOffset,
        std::min(blockCount, numBlocks - blockOffset));
}

bool FlashAllocSpan::flashAddrToOffset(FlashAddr flockAddr, ByteOffset &byteOffset) const
{
    return false;
}

bool FlashAllocSpan::offsetToFlashAddr(ByteOffset byteOffset, FlashAddr &flockAddr) const
{
    return false;
}

bool FlashAllocSpan::offsetIsValid(ByteOffset byteOffset) const
{
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
