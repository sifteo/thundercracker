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

FlashLFSVolumeHeader *FlashLFSVolumeHeader::fromVolume(FlashBlockRef &ref, FlashVolume vol)
{
    /*
     * Obtain a mapped FlashLFSVolumeHeader pointer from a FlashVolume.
     * Requires that the FlashVolume was created with enough type-specific
     * data space allocated for this structure. Returns 0 on error.
     */

    unsigned size;
    uint8_t *data = vol.mapTypeSpecificData(ref, size);
    if (size >= sizeof(FlashLFSVolumeHeader))
        return (FlashLFSVolumeHeader*) data;
    else
        return 0;
}

bool FlashLFSVolumeHeader::isRowEmpty(unsigned row) const
{
    ASSERT(row < NUM_ROWS);
    return metaIndex[row].isEmpty();
}

void FlashLFSVolumeHeader::add(unsigned row, unsigned key) {
    metaIndex[row].add(row, key);
}

bool FlashLFSVolumeHeader::test(unsigned row, unsigned key) {
    return metaIndex[row].test(row, key);
}

unsigned FlashLFSVolumeHeader::numNonEmptyRows()
{
    /*
     * Count how many non-empty rows there are. The volume header is always
     * partitioned into a set of empty rows following a set of non-empty
     * rows. This is a quick binary search to locate the boundary.
     *
     * In other words, this returns the index of the first empty row, or
     * NUM_ROWS if no rows are still empty.
     */

    // The result is guaranteed to be in the closed interval [first, last].
    unsigned first = 0;
    unsigned last = NUM_ROWS;

    while (first < last) {
        unsigned middle = (first + last) >> 1;
        ASSERT(first <= middle);
        ASSERT(middle < last);
        ASSERT(middle < NUM_ROWS);

        if (isRowEmpty(middle))
            last = middle;
        else
            first = middle + 1;
    }

    ASSERT(first == last);
    ASSERT(first <= NUM_ROWS);
    ASSERT(first == NUM_ROWS || isRowEmpty(first));
    ASSERT(first == 0 || !isRowEmpty(first - 1));
    return first;
}

bool FlashLFSIndexBlockIter::beginBlock(uint32_t blockAddr)
{
    /*
     * Initialize the iterator to the first valid index record in the
     * given block. Returns false if no valid index records exist, or
     * an anchor can't be found.
     */

    FlashBlock::get(blockRef, blockAddr);

    FlashLFSIndexAnchor *ptr = LFS::firstAnchor(&*blockRef);
    FlashLFSIndexAnchor *limit = LFS::lastAnchor(&*blockRef);

    anchor = 0;
    currentRecord = 0;

    while (!ptr->isValid()) {
        if (ptr->isEmpty())
            return false;
        ptr++;
        if (ptr > limit)
            return false;
    }

    anchor = ptr;
    currentOffset = anchor->getOffsetInBytes();
    currentRecord = LFS::firstRecord(ptr);

    if (currentRecord->isValid())
        return true;
    else
        return next();
}

bool FlashLFSIndexBlockIter::prev()
{
    /*
     * Seek to the previous valid index record. Invalid records are skipped.
     * Returns false if no further index records are available.
     */

    FlashLFSIndexRecord *ptr = currentRecord;
    ASSERT(ptr);
    ASSERT(ptr->isValid());

    while (1) {
        ptr--;
        if (ptr < LFS::firstRecord(anchor))
            return false;

        if (ptr->isValid()) {
            // Only valid records have a meaningful size
            currentRecord = ptr;
            currentOffset -= ptr->getSizeInBytes();
            return true;
        }
    }
}

bool FlashLFSIndexBlockIter::next()
{
    /*
     * Seek to the next valid index record. Invalid records are skipped.
     * Returns false if no further index records are available.
     */

    FlashLFSIndexRecord *ptr = currentRecord;
    ASSERT(ptr);
    ASSERT(ptr->isValid());

    unsigned nextOffset = currentOffset + ptr->getSizeInBytes();

    while (1) {
        ptr++;
        if (ptr > LFS::lastRecord(&*blockRef))
            return false;

        if (ptr->isValid()) {
            currentRecord = ptr;
            currentOffset = nextOffset;
            return true;
        }
    }
}

FlashLFS::FlashLFS(FlashVolume parent)
{
}

bool FlashLFS::findObject(unsigned key, uint32_t &addr, uint32_t &size)
{
    // Scan meta-index backwards, for candidate blocks
    // Scan blocks forward, to establish anchor
    // Scan backwards until we match
    // Return address and size of object (guaranteed linear)
    return false;
}

bool FlashLFS::newObject(unsigned key, uint32_t size, uint32_t crc, uint32_t &addr)
{
    // Jump to last block in meta-index. If it's full, allocate a new block and write an anchor
    // Write an index record (allocates space for the object)
    // Return location at which object data can be written.
    // Write the object data (may be split among multiple flash blocks)
    return false;
}

bool FlashLFS::collectGarbage()
{
    // Scan backwards, keeping a bitmap of all keys we've found
    // Any volume consisting of only superceded keys is deleted
    
    // Also: If the oldest volume is less than X amount full, copy
    //       all non-superceded keys, then delete it.
    //       XXX - how to do this for more than one volume at a time?
    return false;
}
