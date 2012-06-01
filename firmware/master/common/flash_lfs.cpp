/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_lfs.h"
#include "macros.h"
#include "bits.h"


uint8_t LFS::computeCheckByte(uint8_t a, uint8_t b)
{
    /*
     * Generate a check-byte value which is dependent on all bits
     * in 'a' and 'b', but is guaranteed not to be 0xFF.
     * (So we can differentiate this from an unprogrammed byte.)
     *
     * Any single-bit error in either 'a' or 'b' is guaranteed
     * to produce a different check byte value, even if we truncate
     * the most significant bit.
     *
     * This pattern of bit rotations means that it's always legal
     * to chop off the MSB. We only actually do this, though, if
     * the result otherwise would have been 0xFF.
     */

    uint8_t result = a ^ ROR8(b, 1) ^ ROR8(a, 3) ^ ROR8(b, 5);

    if (result == 0xFF)
        result = 0x7F;

    return result;
}

bool LFS::isEmpty(const uint8_t *bytes, unsigned count)
{
    // Are the specified bytes all unprogrammed?

    while (count) {
        if (bytes[0] != 0xFF)
            return false;
        bytes++;
        count--;
    }
    return true;
}

#if 0

const FlashLFSIndexAnchor *FlashLFSIndexBlock::findAnchor() const
{
    /*
     * Look for the valid anchor in this index block, if it exists.
     * If no valid anchor exists, returns 0.
     */

    const FlashLFSIndexAnchor *ptr = (const FlashLFSIndexAnchor*) &bytes[0];
    const FlashLFSIndexAnchor *limit = ((const FlashLFSIndexAnchor*) &bytes[sizeof bytes]) - 1;

    while (ptr <= limit) {
        if (ptr->isValid())
            return ptr;
        if (ptr->isEmpty())
            break;
        ptr++;
    }

    return 0;
}

const FlashLFSIndexRecord *FlashLFSIndexBlock::findRecord(
    const FlashLFSIndexAnchor *anchor, unsigned key, unsigned &objectOffset) const
{
    /*
     * Look for the latest record with the specified key. If we find it,
     * objectOffset is set to the byte offset of the corresponding object,
     * and we return a pointer to the record.
     *
     * If no matching record exists, returns zero.
     *
     * "anchor" must be the nonzero result of calling findAnchor() successfully.
     * This intermediate result may be reused by the caller.
     */

    ASSERT(anchor);
    ASSERT(anchor->isValid());
    ASSERT(&objectOffset);
    ASSERT(key < FlashLFSIndexRecord::MAX_KEYS);

    const FlashLFSIndexRecord *ptr = (const FlashLFSIndexRecord*) (anchor + 1);
    const FlashLFSIndexRecord *limit = ((const FlashLFSIndexRecord*) &bytes[sizeof bytes]) - 1;
    unsigned offset = anchor->getOffsetInBytes();

    while (ptr <= limit) {
        if (ptr->isValid()) {
            // Valid index records always have space allocated to them,
            // even if the object CRC is no good
            
            if (ptr->getKey() == key) {
                objectOffset = offset;
                return ptr;
            } else {
                offset += ptr->getSizeInBytes();
            }
        } else if (ptr->isEmpty()) {
            break;
        }
            
            return ptr;

        if (ptr->isEmpty())
            break;
    }

    return 0;
    
}

#endif