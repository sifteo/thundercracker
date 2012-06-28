/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_lfs.h"
#include "macros.h"
#include "bits.h"
#include "crc.h"

FlashLFS FlashLFSCache::instances[SIZE];
uint8_t FlashLFSCache::lastUsed = 0;


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

uint32_t FlashLFSVolumeHeader::getSequence(FlashVolume vol)
{
    FlashBlockRef ref;
    FlashLFSVolumeHeader *hdr = fromVolume(ref, vol);
    return hdr->sequence;
}

bool FlashLFSVolumeHeader::isRowEmpty(unsigned row) const
{
    ASSERT(row < NUM_ROWS);
    return metaIndex[row].isEmpty();
}

bool FlashLFSVolumeHeader::test(unsigned row, unsigned key) {
    return metaIndex[row].overlaps(FlashLFSKeyFilter::makeSingle(row, key));
}

void FlashLFSVolumeHeader::add(unsigned row, unsigned key) {
    metaIndex[row].add(row, key);
}

bool FlashLFSKeyQuery::test(unsigned row, FlashLFSKeyFilter f)
{
    FlashLFSKeyFilter filter;

    if (exactKey != LFS::KEY_ANY) {
        // Filter contains a single matching key
        filter = FlashLFSKeyFilter::makeSingle(row, exactKey);

    } else if (excluded) {
        // Filter contains all keys that *aren't* in 'excluded'

        filter = FlashLFSKeyFilter::makeEmpty();

        for (unsigned key = 0; key < FlashLFSIndexRecord::MAX_KEYS; key++) {
            if (!excluded->test(key))
                filter.add(row, key);
            if (filter.isFull())
                break;
        }

    } else {
        // All keys are free game. We're fine as long as the filter isn't empty.
        return !f.isEmpty();
    }

    return filter.overlaps(f);
}

bool FlashLFSKeyQuery::test(unsigned key)
{
    if (exactKey != LFS::KEY_ANY)
        return key == exactKey;

    if (excluded && excluded->test(key))
        return false;

    return true;
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

bool FlashLFSIndexBlockIter::previous(FlashLFSKeyQuery query)
{
    /*
     * Seek to the previous valid index record. Invalid records are skipped.
     * Returns false if no further index records are available.
     *
     * If 'key' is not KEY_ANY, skips records that don't match this key.
     */

    FlashLFSIndexRecord *ptr = currentRecord;
    unsigned offset = currentOffset;

    ASSERT(ptr);
    ASSERT(ptr->isValid());

    while (1) {
        ptr--;
        if (ptr < LFS::firstRecord(anchor))
            return false;

        if (ptr->isValid()) {
            // Only valid records have a meaningful size
            offset -= ptr->getSizeInBytes();

            if (query.test(ptr->getKey())) {
                // Matched!
                currentRecord = ptr;
                currentOffset = offset;
                return true;
            }
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
            ASSERT(currentOffset == anchor->getOffsetInBytes());
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

void FlashLFSVolumeVector::append(FlashVolume vol, SequenceInfo &si)
{
    if (full()) {
        // Too many volumes!
        ASSERT(0);
        return;
    }

    unsigned index = numSlotsInUse;
    si.slots[index] = FlashLFSVolumeHeader::getSequence(vol);
    slots[index] = vol;
    numSlotsInUse = index + 1;
}

void FlashLFSVolumeVector::compact()
{
    /*
     * Look for invalid (deleted) volumes to remove,
     * in order to make more room in our vector.
     */

    DEBUG_ONLY(debugChecks());

    unsigned readIdx = 0;
    unsigned writeIdx = 0;

    while (readIdx < numSlotsInUse) {
        FlashVolume vol = slots[readIdx++];
        if (vol.block.isValid())
            slots[writeIdx++] = vol;
    }

    numSlotsInUse = writeIdx;
    DEBUG_ONLY(debugChecks());
}

void FlashLFSVolumeVector::sort(SequenceInfo &si)
{
    /*
     * Bubble sort actually shouldn't be too bad here... it should be common
     * for volumes to already be sorted, based on the FlashVolume allocator's
     * algorithms. And besides, with so few elements, I care more about
     * memory than CPU.
     *
     * (Also, qsort() is kind of ugly for this use case, and newlib
     * doesn't have qsort_r() yet. Blah.)
     */

    DEBUG_ONLY(debugChecks());

    for (unsigned limit = numSlotsInUse; limit;) {
        unsigned nextLimit = 0;
        for (unsigned i = 1; i < limit; i++)
            if (si.slots[i-1] > si.slots[i]) {
                swap(si.slots[i-1], si.slots[i]);
                swap(slots[i-1], slots[i]);
                nextLimit = i;
            }
        limit = nextLimit;
    }

    DEBUG_ONLY(debugChecks());
}

#ifdef SIFTEO_SIMULATOR
void FlashLFSVolumeVector::debugChecks()
{
    /*
     * Make sure all volumes in the vector are unique and valid.
     */

    ASSERT(numSlotsInUse <= MAX_VOLUMES);

    for (unsigned i = 0; i < numSlotsInUse; ++i) {
        ASSERT(!slots[i].block.isValid() || slots[i].isValid());
        for (unsigned j = 0; j < i; ++j)
            ASSERT(slots[i].block.code != slots[j].block.code);
    }
}
#endif

void FlashLFS::init(FlashVolume parent)
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
    FlashLFSVolumeVector::SequenceInfo si;

    volumes.clear();
    vi.begin();
    while (vi.next(vol)) {
        if (vol.getParent().block.code == parent.block.code && vol.getType() == FlashVolume::T_LFS)
            volumes.append(vol, si);
    }

    initWithVolumeVector(parent, si);
}

void FlashLFS::initWithVolumeVector(FlashVolume parent, FlashLFSVolumeVector::SequenceInfo &si)
{
    /*
     * After we have set up our 'parent', FlashLFSVolumeVector and SequenceInfo,
     * this function takes over and finishes all remaining initialization.
     */

    this->parent = parent;

    volumes.sort(si);

    unsigned index = volumes.numSlotsInUse;
    lastSequenceNumber = index ? si.slots[index - 1] : 0;
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

FlashLFS &FlashLFSCache::get(FlashVolume parent)
{
    ASSERT(lastUsed < SIZE);

    for (unsigned i = 0; i < SIZE; ++i) {
        FlashLFS &lfs = instances[i];
        if (lfs.isMatchFor(parent)) {
            lastUsed = i;
            return lfs;
        }
    }

    // Cache miss
    lastUsed = (lastUsed + 1) % SIZE;
    FlashLFS &lfs = instances[lastUsed];
    lfs.init(parent);
    ASSERT(lfs.isMatchFor(parent));
    return lfs;
}

void FlashLFSCache::invalidate()
{
    ASSERT(lastUsed < SIZE);
    for (unsigned i = 0; i < SIZE; ++i)
        instances[i].invalidate();
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

bool FlashLFSObjectAllocator::allocateAndCollectGarbage()
{
    return allocate() || (lfs.collectGarbage() && allocate());
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

    ASSERT((size % FlashLFSIndexRecord::SIZE_UNIT) == 0);

    FlashBlockWriter writer;
    FlashLFSIndexBlockIter iter;
    uint32_t indexBlockAddr = LFS::indexBlockAddr(vol, row);
    unsigned volBaseAddr = vol.block.address() + FlashBlock::BLOCK_SIZE;

    if (!iter.beginBlock(indexBlockAddr)) {
        // Write an anchor to this block

        /*
         * Before we start writing to the index block, we need to make sure
         * that there's even room for the block at all. In the worst case, this
         * block may already be full of payload data and we'd be overwriting it
         * if we tried to allocate this as an index block!
         *
         * So, this is similar to the space calculation below, but it's more
         * conservative: we just need to know if there's any way that it isn't
         * a bad idea to allocate this index block.
         */

        uint32_t anchorOffset = findAnchorOffset(vol, row);
        if (volBaseAddr + anchorOffset + size > indexBlockAddr)
            return false;

        writer.beginBlock(indexBlockAddr);
        writeAnchor(writer, anchorOffset);

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

    addr = volBaseAddr + iter.getCurrentOffset();
    if (addr + size > writer.ref->getAddress())
        return false;

    // Finish writing the record
    newRecord->init(key, size, crc);

    // Write to the meta-index's FlashLFSKeyFilter for this row.
    if (!hdr->test(row, key)) {
        writer.beginBlock(&*hdrRef);
        hdr->add(row, key);
    }

    return true;
}

// Start just past the last volume
FlashLFSObjectIter::FlashLFSObjectIter(FlashLFS &lfs)
    : lfs(lfs), volumeCount(lfs.volumes.numSlotsInUse + 1), rowCount(0)
{
    ASSERT(lfs.isValid());
}

bool FlashLFSObjectIter::readAndCheck(uint8_t *buffer, unsigned size) const
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
    uint32_t crc = cs.get(FlashLFSIndexRecord::SIZE_UNIT);

    return record()->checkCRC(crc);
}

bool FlashLFSObjectIter::previous(FlashLFSKeyQuery query)
{
    ASSERT(lfs.isValid());
    ASSERT(volumeCount <= lfs.volumes.numSlotsInUse + 1);
    DEBUG_ONLY(lfs.volumes.debugChecks());

    while (volumeCount) {

        if (rowCount) {
            // We have a valid volume and row. Previous record within a block
            if (indexIter.previous(query))
                return true;

        } else {
            /*
             * Out of rows? Go to the next volume, and reset the index and
             * meta-index iterators to the end of that volume.
             * Sets 'hdr' and 'rowCount'.
             */

            if (!--volumeCount)
                break;

            FlashVolume vol = volume();
            if (!vol.block.isValid()) {
                // Empty slot in our volume array
                continue;
            }
            
            unsigned hdrSize = sizeof(FlashLFSVolumeHeader);
            hdr = (FlashLFSVolumeHeader*) vol.mapTypeSpecificData(hdrRef, hdrSize);
            ASSERT(hdrSize == sizeof(FlashLFSVolumeHeader));

            // Just past the last row
            rowCount = hdr->numNonEmptyRows() + 1;
        }
        
        /*
         * Out of records in this row. Find the next row, skipping any
         * that either don't have a valid anchor, and any we can rule
         * out via the FlashLFSKeyFilter.
         */
        
        while (--rowCount) {
            unsigned i = rowCount - 1;
            if (query.test(i, hdr->metaIndex[i]) &&
                indexIter.beginBlock(LFS::indexBlockAddr(volume(), i))) {

                /*
                 * We have a candidate row which may contain the key we're
                 * looking for. Iterate to the end, then either we'll be
                 * pointing to a matching key, or we can loop back up to
                 * previous() above and we'll find one.
                 */
                
                while (indexIter.next());

                if (query.test(indexIter->getKey()))
                    return true;
                else
                    break;
            }
        }
    }            

    // End of iteration
    ASSERT(volumeCount == 0);
    rowCount = 0;
    hdr = 0;
    return false;
}

bool FlashLFS::collectGarbage()
{
    ASSERT(isValid());

    // Fast path: can we collect garbage from our own local volume?
    if (collectLocalGarbage())
        return true;

    // Nope, try every other volume.
    return collectGlobalGarbage(this);
}

bool FlashLFS::collectGlobalGarbage(FlashLFS *exclude)
{
    /*
     * Next best: collect garbage from any of the FlashLFS instances
     * in our cache. (Assuming they aren't identical to the excluded LFS)
     */
    for (unsigned i = 0; i < FlashLFSCache::SIZE; ++i) {
        FlashLFS &lfs = FlashLFSCache::instances[i];
        if (&lfs != exclude && lfs.isValid() && lfs.collectLocalGarbage())
            return true;
    }

    /*
     * Okay, now we need to scrape through every other filesystem in flash.
     *
     * It's important that we avoid filesystems we've already checked.
     * In addition to the obvious desire not to duplicate work, we cannot
     * ever have two FlashLFS instances for the same filesystem, or they
     * could become inconsistent with each other.
     */

    FlashMapBlock::ISet blacklist;
    blacklist.clear();

    if (exclude)
        exclude->parent.block.mark(blacklist);

    for (unsigned i = 0; i < FlashLFSCache::SIZE; ++i) {
        FlashLFS &lfs = FlashLFSCache::instances[i];
        if (lfs.isValid())
            lfs.parent.block.mark(blacklist);
    }

    /*
     * Repeatedly build a FlashLFS instance for the first non-blacklisted
     * LFS instance we find. This is a variant of FlashLFS::init() which
     * builds the LFS and selects in in a single step.
     *
     * We're finished if we reach the end of the filesystem without finding
     * another non-blacklisted LFS volume.
     */

    for (;;) {
        FlashVolumeIter vi;
        FlashVolume vol;
        FlashLFSVolumeVector::SequenceInfo si;
        FlashLFS iterLFS;
        FlashVolume iterParent;

        iterLFS.volumes.clear();
        vi.begin();

        // Search for the first volume on any parent
        for (;;) {
            if (!vi.next(vol))
                return false;

            if (vol.getType() != FlashVolume::T_LFS)
                continue;

            iterParent = vol.getParent();
            if (!iterParent.block.test(blacklist))
                break;
        }
        iterLFS.volumes.append(vol, si);

        // Continue searching for volumes that are part of this same LFS
        while (vi.next(vol)) {
            if (vol.getParent().block.code == iterParent.block.code && vol.getType() == FlashVolume::T_LFS)
                iterLFS.volumes.append(vol, si);
        }

        iterLFS.initWithVolumeVector(iterParent, si);
        if (iterLFS.collectLocalGarbage())
            return true;

        // No luck, try a diferent LFS instance.
        iterParent.block.mark(blacklist);
    }
}

bool FlashLFS::collectLocalGarbage()
{
    ASSERT(isValid());

    /*
     * Iterate through this LFS, from newest to oldest, keeping track of which
     * volume we're in and which keys have already been "obsoleted" by newer
     * versions.
     *
     * Any volume which has no non-obsolete keys can be safely deleted with
     * no additional work.
     *
     * Any volume which has a high proportion of obsolete data can be deleted
     * after any non-obsolete keys are copied.
     */

    // Early out
    if (volumes.numSlotsInUse == 0)
        return false;

    // Marked bits indicate volumes not to delete
    BitVector<FlashLFSVolumeVector::MAX_VOLUMES> volumesToKeep;
    volumesToKeep.clear();

    // Keys for records which have been obsoleted by newer ones
    FlashLFSIndexRecord::KeyVector_t obsoleteKeys;
    obsoleteKeys.clear();

    // Iterate over non-obsolete keys only
    FlashLFSObjectIter iter(*this);
    while (iter.previous(FlashLFSKeyQuery(&obsoleteKeys))) {
        const FlashLFSIndexRecord *record = iter.record();
        unsigned key = record->getKey();

        // Any additional instances of this key are obsolete
        ASSERT(obsoleteKeys.test(key) == false);
        obsoleteKeys.mark(key);

        // Keep this volume
        volumesToKeep.mark(iter.volumeIndex());
    }

    // Delete obsolete volumes
    for (unsigned i = 0; i < volumes.numSlotsInUse; ++i) {
        if (!volumesToKeep.test(i)) {
            FlashVolume &vol = volumes.slots[i];
            vol.deleteSingle();
            volumes.slots[i].block.setInvalid();
        }
    }

    return false;
}
