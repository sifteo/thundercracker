/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_lfs.h"
#include "macros.h"
#include "bits.h"
#include "crc.h"


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
     *
     * XOR'ing in a constant ensures that a run of zeroes isn't
     * misinterpreted as valid data.
     */

    uint8_t result = 0x42 ^ a ^ ROR8(b, 1) ^ ROR8(a, 3) ^ ROR8(b, 5);

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

uint32_t LFS::indexBlockAddr(FlashVolume vol, unsigned row)
{
    // Calculate the raw flash address of an index block

    ASSERT(vol.isValid());
    ASSERT(vol.getType() == FlashVolume::T_LFS);
    ASSERT(row < FlashLFSVolumeHeader::NUM_ROWS);

    return vol.block.address()
        + FlashMapBlock::BLOCK_SIZE
        - (row + 1) * FlashBlock::BLOCK_SIZE;
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

    // Point just prior to the first record
    anchor = ptr;
    currentOffset = anchor->getOffsetInBytes();
    currentRecord = 0;
    return true;
}

bool FlashLFSIndexBlockIter::previous(unsigned key)
{
    /*
     * Seek to the previous valid index record. Invalid records are skipped.
     * Returns false if no further index records are available.
     *
     * If 'key' is not KEY_ANY, skips records that don't match this key.
     */

    FlashLFSIndexRecord *ptr = currentRecord;
    ASSERT(ptr);
    ASSERT(ptr->isValid());

    while (1) {
        ptr--;
        if (ptr < LFS::firstRecord(anchor))
            return false;

        if (ptr->isValid() && (key == LFS::KEY_ANY || key == ptr->getKey())) {
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
    unsigned nextOffset = currentOffset;

    if (ptr) {
        // Already pointing to a valid record
        ASSERT(ptr->isValid());
        nextOffset += ptr->getSizeInBytes();

    } else {
        // Iterating from anchor to first record
        ptr = LFS::firstRecord(anchor);
        if (ptr->isValid()) {
            currentRecord = ptr;
            return true;
        }
    }

    while (1) {
        ptr++;
        if (ptr > LFS::lastRecord(&*blockRef))
            return false;

        if (ptr->isEmpty())
            return false;

        if (ptr->isValid()) {
            currentRecord = ptr;
            currentOffset = nextOffset;
            return true;
        }
    }
}

FlashLFSIndexRecord *FlashLFSIndexBlockIter::beginAppend(FlashBlockWriter &writer)
{
    /*
     * Start writing to the first unused record slot in this index block.
     * If we're out of space, returns 0. Otherwise, points 'writer' at the
     * proper block and returns a writable mapped pointer to the new record.
     *
     * Advances the iterator to this new record.
     */

    // Advance through all valid records
    while (next());

    FlashLFSIndexRecord *ptr = currentRecord;
    unsigned nextOffset = currentOffset;

    if (ptr) {
        // Pointing to a valid record; advance past it.
        ASSERT(ptr->isValid());
        nextOffset += ptr->getSizeInBytes();
        ptr++;
    } else {
        // Empty! Start at the beginning.
        ptr = LFS::firstRecord(anchor);
        ASSERT(!ptr->isValid());
        ASSERT(nextOffset == anchor->getOffsetInBytes());
    }

    // Skip any invalid records until we get to the first empty one.
    do {
        if (ptr > LFS::lastRecord(&*blockRef))
            return 0;
    } while (!ptr->isEmpty());

    // Write here.
    writer.beginBlock(&*blockRef);
    currentRecord = ptr;
    currentOffset = nextOffset;
    return ptr;
}

void FlashLFSVolumeVector::compact()
{
    /*
     * Look for invalid (deleted) volumes to remove,
     * in order to make more room in our vector.
     */

    unsigned readIdx = 0;
    unsigned writeIdx = 0;

    while (readIdx < numSlotsInUse) {
        FlashVolume vol = slots[readIdx++];
        if (vol.block.isValid())
            slots[writeIdx++] = vol;
    }

    numSlotsInUse = writeIdx;
}

FlashLFS::FlashLFS(FlashVolume parent)
    : parent(parent)
{
    /*
     * Locate all of the existing LFS volumes under this parent.
     * We need to:
     *
     *  1) Filter all volumes by type and parent, to locate the ones
     *     that are part of this LFS.
     *  2) Sort all volumes in order of ascending sequence number
     *  3) Save the sorted volumes as well as the peak sequence number
     *
     * Because the list of volumes is so tiny (only 13 currently) we don't
     * need any fancy methods to store the sequence numbers or sort the
     * array. 
     */

    FlashVolumeIter vi;
    FlashVolume vol;

    // Store sequence numbers on the stack (Parallel to the volumes array)
    uint32_t sequences[FlashLFSVolumeVector::MAX_VOLUMES];
    unsigned index = 0;

    vi.begin();
    while (vi.next(vol)) {
        if (vol.getParent().block.code == parent.block.code
            && vol.getType() == FlashVolume::T_LFS) {

            if (index == FlashLFSVolumeVector::MAX_VOLUMES) {
                // Too many volumes!
                ASSERT(0);
                break;
            }

            // Found a matching volume. Look at its sequence number.
            FlashBlockRef ref;
            unsigned hdrSize = sizeof(FlashLFSVolumeHeader);
            FlashLFSVolumeHeader *hdr = (FlashLFSVolumeHeader*)vol.mapTypeSpecificData(ref, hdrSize);
            ASSERT(hdrSize == sizeof(FlashLFSVolumeHeader));
            uint32_t volSequence = hdr->sequence;

            // Store volume info
            sequences[index] = hdr->sequence;
            volumes.slots[index] = vol;
            index++;
        }
    }

    volumes.numSlotsInUse = index;
    if (index == 0) {
        lastSequenceNumber = 0;
        return;
    }

    /*
     * Bubble sort actually shouldn't be too bad here... it should be common
     * for volumes to already be sorted, based on the FlashVolume allocator's
     * algorithms. And besides, with so few elements, I care more about
     * memory than CPU.
     *
     * (Also, qsort() is kind of ugly for this use case, and newlib
     * doesn't have qsort_r() yet. Blah.)
     */

    for (unsigned limit = index; limit;) {
        unsigned nextLimit = 0;
        for (unsigned i = 1; i < limit; i++)
            if (sequences[i-1] > sequences[i]) {
                swap(sequences[i-1], sequences[i]);
                swap(volumes.slots[i-1], volumes.slots[i]);
                nextLimit = i;
            }
        limit = nextLimit;
    }

    lastSequenceNumber = sequences[index - 1];
}

bool FlashLFS::newVolume()
{
    /*
     * Allocate a new volume with the next sequence number.
     *
     * Fails if we have too many volumes in our vector, or if there
     * isn't any more space to allocate volumes.
     */

    if (volumes.full()) {
        // Try to reclaim wasted space in our vector
        volumes.compact();
        if (volumes.full())
            return false;
    }

    FlashVolumeWriter vw;
    unsigned hdrSize = sizeof(FlashLFSVolumeHeader);
    unsigned payloadSize = FlashLFSVolumeVector::VOL_PAYLOAD_SIZE;
    
    if (!vw.begin(FlashVolume::T_LFS, payloadSize, hdrSize, parent))
        return false;

    FlashLFSVolumeHeader *hdr = (FlashLFSVolumeHeader*)vw.mapTypeSpecificData(hdrSize);
    ASSERT(hdrSize == sizeof(FlashLFSVolumeHeader));

    hdr->sequence = ++lastSequenceNumber;
    vw.commit();
    volumes.append(vw.volume);

    return true;
}

FlashLFSObjectAllocator::FlashLFSObjectAllocator(FlashLFS &lfs, unsigned key,
    unsigned size, unsigned crc)
    : lfs(lfs), key(key),
      size(roundup<FlashLFSIndexRecord::SIZE_UNIT>(size)),
      crc(crc), addr(FlashBlock::INVALID_ADDRESS)
{
    ASSERT(FlashLFSIndexRecord::isKeyAllowed(key));
    ASSERT(FlashLFSIndexRecord::isSizeAllowed(size));
    ASSERT(FlashLFSIndexRecord::isSizeAllowed(this->size));
}

bool FlashLFSObjectAllocator::allocate()
{
    /*
     * Allocate space for a new object, and write an index record for it.
     * We delegate this to allocInVolume() if we have a candidate volume
     * to write to. Otherwise, we'll try to allocate a new volume.
     */

    FlashVolume vol = lfs.volumes.last();
    if (vol.block.isValid() && allocInVolume(vol))
        return true;

    return lfs.newVolume() && allocInVolume(lfs.volumes.last());
}

bool FlashLFSObjectAllocator::allocInVolume(FlashVolume vol)
{
    /*
     * Try to allocate a new object within a specific volume. This looks for
     * the last meta-index row, and tries to allocate a volume there. If
     * there isn't enough space, we try again on a new meta-index row.
     */

    ASSERT(vol.isValid());

    // Map the FlashVolume header
    FlashBlockRef hdrRef;
    unsigned hdrSize = sizeof(FlashLFSVolumeHeader);
    FlashLFSVolumeHeader *hdr = (FlashLFSVolumeHeader*)vol.mapTypeSpecificData(hdrRef, hdrSize);
    ASSERT(hdrSize == sizeof(FlashLFSVolumeHeader));

    // How full is this volume?
    unsigned numNonEmptyRows = hdr->numNonEmptyRows();
    ASSERT(numNonEmptyRows <= FlashLFSVolumeHeader::NUM_ROWS);

    // Can we add it to the last row?
    if (numNonEmptyRows && allocInVolumeRow(vol, hdrRef, hdr, numNonEmptyRows - 1))
        return true;

    // Okay, how about a new row?
    return (numNonEmptyRows < FlashLFSVolumeHeader::NUM_ROWS)
        && allocInVolumeRow(vol, hdrRef, hdr, numNonEmptyRows);
}

uint32_t FlashLFSObjectAllocator::findAnchorOffset(FlashVolume vol, unsigned row)
{
    /*
     * Look for the proper anchor offset to set in the index block for 'row'.
     * Assumes 'row' is a new, empty index block. We search backwards for an
     * index block that we can calculate a valid anchor offset from.
     */

    while (row) {
        // Examine the previous row.
        row--;

        FlashLFSIndexBlockIter iter;
        if (iter.beginBlock(LFS::indexBlockAddr(vol, row))) {
            // This row has a valid anchor. Iterate to the end.
            while (iter.next());
            return iter.getNextOffset();
        }
    }

    // Base case: No previous objects, start at zero.
    return 0;
}

void FlashLFSObjectAllocator::writeAnchor(FlashBlockWriter &writer, uint32_t anchorOffset)
{
    /*
     * Using a FlashBlockWriter that's already pointing at an index block,
     * try to write an anchor into that block. Fails silently; we detect
     * failures in FlashLFSIndexBlockIter.
     */

    FlashLFSIndexAnchor *ptr = LFS::firstAnchor(&*writer.ref);
    FlashLFSIndexAnchor *limit = LFS::lastAnchor(&*writer.ref);

    while (ptr <= limit) {
        // Shouldn't already have an anchor!
        ASSERT(!ptr->isValid());

        if (ptr->isEmpty()) {
            ptr->init(anchorOffset);
            return;
        }

        ptr++;
    }
}

bool FlashLFSObjectAllocator::allocInVolumeRow(FlashVolume vol,
    FlashBlockRef &hdrRef, FlashLFSVolumeHeader *hdr, unsigned row)
{
    /*
     * Try to allocate a new object, with a specific volume and
     * index block (meta-index row). The specified row is guaranteed
     * to either be the last one in the volume, or a blank new row.
     *
     * We need to return false if either the index block is full or
     * the volume has no room for both the object and the current
     * index block.
     */

    /*
     * First thing's first: Make sure this index block has an anchor.
     * If not, we'll have to scan the previous block in order to
     * compute the proper anchor offset.
     */

    FlashBlockWriter writer;
    FlashLFSIndexBlockIter iter;
    uint32_t indexBlockAddr = LFS::indexBlockAddr(vol, row);

    if (!iter.beginBlock(indexBlockAddr)) {
        // Write an anchor to this block

        writer.beginBlock(indexBlockAddr);
        writeAnchor(writer, findAnchorOffset(vol, row));

        if (!iter.beginBlock(indexBlockAddr)) {
            /*
             * We couldn't write (or successfully read) the anchor.
             * This really should never happen in normal operation, but
             * in theory this could mean we had power failures during
             * every attempt at using this block, and we've filled it up
             * with invalid anchors. ASSERT on emulator builds.
             */
            ASSERT(0);
            return false;
        }
    }

    // Point to the last record in this index block
    while (iter.next());

    // Try to write an index record; we'll fail if the block is full.
    FlashLFSIndexRecord *newRecord = iter.beginAppend(writer);
    if (!newRecord)
        return false;

    /*
     * Now we can calculate the offset of this new object. Make
     * sure we haven't filled up the volume; if we have, we'll see
     * a collision between the object addresses (growing up) and the
     * index blocks (growing down).
     */

    ASSERT((size % FlashLFSIndexRecord::SIZE_UNIT) == 0);
    addr = vol.block.address() + FlashBlock::BLOCK_SIZE + iter.getCurrentOffset();
    if (addr + size > writer.ref->getAddress())
        return false;

    // Finish writing the record
    newRecord->init(key, size, crc);

    // Write to the meta-index's FlashLFSKeyFilter for this row.
    writer.beginBlock(&*hdrRef);
    hdr->add(row, key);

    return true;
}

// Start just prior to the first volume
FlashLFSObjectIter::FlashLFSObjectIter(FlashLFS &lfs)
    : lfs(lfs), volumeCount(lfs.volumes.numSlotsInUse + 1), rowCount(0)
{}

bool FlashLFSObjectIter::readAndCheck(uint8_t *buffer, unsigned size)
{
    /*
     * Read 'size' bytes from the current address in flash into 'buffer',
     * CRC them, and return 'true' iff the CRC matches.
     *
     * Generally this will be the buffer we're reading an object into,
     * therefore avoiding a separate copy or any cache pollution. The
     * buffer may not generally be smaller than the stored object. This
     * is only permissible if the truncated object matches the original
     * object when padded out to a SIZE_UNIT boundary with 0xFF bytes,
     * as you'd see when reading or writing an object which is not a
     * multiple of our SIZE_UNIT.
     */

    FlashDevice::read(address(), buffer, size);

    CrcStream cs;
    cs.reset();
    cs.addBytes(buffer, size);
    return record()->checkCRC(cs.get(FlashLFSIndexRecord::SIZE_UNIT));
}

bool FlashLFSObjectIter::previous(unsigned key)
{
    while (volumeCount) {

        if (rowCount) {
            // We have a valid volume and row. Previous record within a block
            if (indexIter.previous(key))
                return true;

            /*
             * Out of records in this row. Find the next row, skipping any
             * that either don't have a valid anchor, and any we can rule
             * out via the FlashLFSKeyFilter.
             *
             * This loop ends with rowCount==0 if no rows could possibly have
             * the key, or it ends with a nonzero rowCount and indexIter pointing
             * just before the first record in this index block. In the latter
             * case, we'll loop back around to the call to previous() above.
             */
            
            while (--rowCount) {
                unsigned i = rowCount - 1;
                if ((key == LFS::KEY_ANY || hdr->test(i, key)) &&
                    indexIter.beginBlock(LFS::indexBlockAddr(volume(), i)))
                    break;
            }

        } else {
            /*
             * Out of rows? Go to the next volume, and reset the index and
             * meta-index iterators to the end of that volume.
             * Sets 'hdr' and 'rowCount'.
             */

            volumeCount--;

            unsigned hdrSize = sizeof(FlashLFSVolumeHeader);
            hdr = (FlashLFSVolumeHeader*) volume().mapTypeSpecificData(hdrRef, hdrSize);
            ASSERT(hdrSize == sizeof(FlashLFSVolumeHeader));

            rowCount = hdr->numNonEmptyRows();
        }
    }            

    // End of iteration
    ASSERT(volumeCount == 0);
    rowCount = 0;
    hdr = 0;
    return false;
}
