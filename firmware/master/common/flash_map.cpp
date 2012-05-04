/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_map.h"
#include <algorithm>


FlashMapSpan FlashMapSpan::split(unsigned blockOffset, unsigned blockCount) const
{
    if (blockOffset >= numBlocks)
        return empty();
    else
        return create(map, firstBlock + blockOffset,
            std::min(blockCount, numBlocks - blockOffset));
}

FlashMapSpan FlashMapSpan::splitRoundingUp(unsigned byteOffset, unsigned byteCount) const
{
    ASSERT((byteOffset & FlashBlock::BLOCK_MASK) == 0);
    return split(byteOffset / FlashBlock::BLOCK_SIZE,
        (byteCount + FlashBlock::BLOCK_MASK) / FlashBlock::BLOCK_SIZE);
}

bool FlashMapSpan::flashAddrToOffset(FlashAddr flashAddr, ByteOffset &byteOffset) const
{
    if (!map)
        return false;

    // Split the flash address
    uint32_t allocBlock = flashAddr / FlashMapBlock::BLOCK_SIZE;
    uint32_t allocOffset = flashAddr & FlashMapBlock::BLOCK_MASK;

    // We have no direct index for this, so do a linear search in our map
    for (unsigned i = 0; i < arraysize(map->blocks); i++)
        if (map->blocks[i].id == allocBlock) {
            ByteOffset result = i * FlashMapBlock::BLOCK_SIZE + allocOffset - firstByte();
            if (offsetIsValid(result)) {
                byteOffset = result;
                return true;
            }
            return false;
        }

    // No match
    return false;
}

bool FlashMapSpan::offsetToFlashAddr(ByteOffset byteOffset, FlashAddr &flashAddr) const
{
    if (!map)
        return false;

    if (!offsetIsValid(byteOffset))
        return false;

    // Split the byte address
    ASSERT(byteOffset < FlashMap::NUM_BYTES);
    ByteOffset absoluteByte = byteOffset + firstByte();
    ByteOffset allocBlock = absoluteByte / FlashMapBlock::BLOCK_SIZE;
    ByteOffset allocOffset = absoluteByte & FlashMapBlock::BLOCK_MASK;

    ASSERT(allocBlock < arraysize(map->blocks));
    flashAddr = map->blocks[allocBlock].address() + allocOffset;

    // Test round-trip mapping
    DEBUG_ONLY({
        ByteOffset b;
        ASSERT(flashAddrToOffset(flashAddr, b) && b == byteOffset);
    });

    return true;
}

bool FlashMapSpan::getBlock(FlashBlockRef &ref, ByteOffset byteOffset) const
{
    ASSERT((byteOffset & FlashBlock::BLOCK_MASK) == 0);

    FlashAddr fa;
    if (!offsetToFlashAddr(byteOffset, fa))
        return false;

    FlashBlock::get(ref, fa);
    return true;
}

bool FlashMapSpan::getBytes(FlashBlockRef &ref, ByteOffset byteOffset, PhysAddr &ptr, uint32_t &length) const
{
    ByteOffset blockPart = byteOffset & ~(ByteOffset)FlashBlock::BLOCK_MASK;
    ByteOffset bytePart = byteOffset & FlashBlock::BLOCK_MASK;
    uint32_t maxLength = FlashBlock::BLOCK_SIZE - bytePart;

    if (!getBlock(ref, blockPart))
        return false;

    ptr = ref->getData() + bytePart;
    length = std::min(length, maxLength);
    return true;
}

bool FlashMapSpan::getByte(FlashBlockRef &ref, ByteOffset byteOffset, PhysAddr &ptr) const
{
    ByteOffset blockPart = byteOffset & ~(ByteOffset)FlashBlock::BLOCK_MASK;
    ByteOffset bytePart = byteOffset & FlashBlock::BLOCK_MASK;

    if (!getBlock(ref, blockPart))
        return false;

    ptr = ref->getData() + bytePart;
    return true;
}

bool FlashMapSpan::preloadBlock(ByteOffset byteOffset) const
{
    FlashAddr flashAddr;

    if (offsetToFlashAddr(byteOffset, flashAddr)) {
        FlashBlock::preload(flashAddr);
        return true;
    }

    return false;
}

bool FlashMapSpan::copyBytes(FlashBlockRef &ref, ByteOffset byteOffset, uint8_t *dest, uint32_t length) const
{
    while (length) {
        PhysAddr srcPA;
        uint32_t chunk = length;

        if (!getBytes(ref, byteOffset, srcPA, chunk))
            return false;

        memcpy(dest, srcPA, chunk);
        dest += chunk;
        byteOffset += chunk;
        length -= chunk;
    }

    return true;
}

bool FlashMapSpan::copyBytes(ByteOffset byteOffset, uint8_t *dest, uint32_t length) const
{
    FlashBlockRef ref;
    return copyBytes(ref, byteOffset, dest, length);
}

bool FlashMapSpan::copyBytesUncached(ByteOffset byteOffset, uint8_t *dest, uint32_t length) const
{
    while (length) {
        /*
         * Note: Even though we are bypassing the cache, we still split on
         *       cache block boundaries, since that's also the alignment
         *       guaranteed by a FlashMapSpan.
         */

        ByteOffset blockPart = byteOffset & ~(ByteOffset)FlashBlock::BLOCK_MASK;
        ByteOffset bytePart = byteOffset & FlashBlock::BLOCK_MASK;
        uint32_t maxLength = FlashBlock::BLOCK_SIZE - bytePart;
        uint32_t chunk = MIN(length, maxLength);

        FlashAddr fa;
        if (!offsetToFlashAddr(blockPart, fa))
            return false;

        FlashDevice::read(fa + bytePart, dest, chunk);

        byteOffset += chunk;
        dest += chunk;
        length -= chunk;
    }

    return true;
}
