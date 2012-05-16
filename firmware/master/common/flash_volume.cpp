/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_volume.h"
#include "flash_volumeheader.h"


bool FlashVolume::isValid() const
{
    ASSERT(this);
    if (!block.isValid())
        return false;

    FlashBlockRef hdrRef;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(hdrRef, block);

    if (!hdr->isHeaderValid())
        return false;

    /*
     * Check CRCs. Note that the Map is not checked if this is
     * a deleted volume, since we selectively invalidate map entries
     * as they are recycled.
     */

    if (hdr->type != T_DELETED && hdr->crcMap != hdr->calculateMapCRC())
        return false;
    if (hdr->crcErase != hdr->calculateEraseCountCRC(block))
        return false;

    return true;
}

unsigned FlashVolume::getType() const
{
    ASSERT(isValid());
    FlashBlockRef ref;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, block);
    ASSERT(hdr->isHeaderValid());
    return hdr->type;
}

FlashMapSpan FlashVolume::getPayload(FlashBlockRef &ref) const
{
    ASSERT(isValid());
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, block);
    ASSERT(hdr->isHeaderValid());

    unsigned size = hdr->payloadBlocks;
    unsigned offset = hdr->payloadOffsetBlocks();
    const FlashMap* map = hdr->getMap();

    return FlashMapSpan::create(map, offset, size);
}

void FlashVolume::markAsDeleted() const
{
    FlashBlockRef ref;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, block);
    ASSERT(hdr->isHeaderValid());

    FlashBlockWriter writer(ref);
    hdr->type = T_DELETED;
    hdr->typeCopy = T_DELETED;
}

bool FlashVolumeIter::next(FlashVolume &vol)
{
    unsigned index;

    ASSERT(initialized == true);

    while (remaining.clearFirst(index)) {
        FlashVolume v(FlashMapBlock::fromIndex(index));

        if (v.isValid()) {
            FlashBlockRef ref;
            FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, v.block);
            ASSERT(hdr->isHeaderValid());
            const FlashMap *map = hdr->getMap();

            // Don't visit any future blocks that are part of this volume
            for (unsigned I = 0, E = hdr->numMapEntries(); I != E; ++I) {
                FlashMapBlock block = map->blocks[I];
                if (block.isValid())
                    block.clear(remaining);
            }

            vol = v;
            return true;
        }
    }

    return false;
}

FlashBlockRecycler::FlashBlockRecycler()
{
    ASSERT(!dirtyVolume.ref.isHeld());
    findOrphansAndDeletedVolumes();
    findCandidateVolumes();
}

void FlashBlockRecycler::findOrphansAndDeletedVolumes()
{
    /*
     * Iterate over volumes once, and calculate sets of orphan blocks,
     * deleted volumes, and calculate an average erase count.
     */

    orphanBlocks.mark();
    deletedVolumes.clear();

    uint64_t avgEraseNumerator = 0;
    uint32_t avgEraseDenominator = 0;

    FlashVolumeIter vi;
    FlashVolume vol;

    vi.begin();
    while (vi.next(vol)) {

        FlashBlockRef ref;
        FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, vol.block);
        ASSERT(hdr->isHeaderValid());

        if (hdr->type == FlashVolume::T_DELETED || hdr->type == FlashVolume::T_INCOMPLETE) {
            // Remember deleted or incomplete recyclable volumes according to their header block
            vol.block.mark(deletedVolumes);
        }

        // If a block is reachable at all, even by a deleted volume, it isn't orphaned.
        // Also calculate the average erase count for all mapped blocks

        const FlashMap *map = hdr->getMap();
        FlashBlockRef eraseRef;

        for (unsigned I = 0, E = hdr->numMapEntries(); I != E; ++I) {
            FlashMapBlock block = map->blocks[I];
            if (block.isValid()) {
                block.clear(orphanBlocks);
                avgEraseNumerator += hdr->getEraseCount(eraseRef, vol.block, I);
                avgEraseDenominator++;
            }
        }
    }

    // If every block is orphaned, it's important to default to a count of zero
    averageEraseCount = avgEraseDenominator ?
        (avgEraseNumerator / avgEraseDenominator) : 0;
}

void FlashBlockRecycler::findCandidateVolumes()
{
    /*
     * Using the results of findOrphansAndDeletedVolumes(),
     * create a set of "candidate" volumes, in which at least one
     * of the valid blocks has an erase count <= the average.
     */

    candidateVolumes.clear();

    FlashMapBlock::Set iterSet = deletedVolumes;
    unsigned index;
    while (iterSet.clearFirst(index)) {

        FlashBlockRef ref;
        FlashMapBlock block = FlashMapBlock::fromIndex(index);
        FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, block);
        const FlashMap *map = hdr->getMap();
        FlashBlockRef eraseRef;

        ASSERT(FlashVolume(block).isValid());
        ASSERT(hdr->type == FlashVolume::T_DELETED || hdr->type == FlashVolume::T_INCOMPLETE);
        ASSERT(hdr->isHeaderValid());

        for (unsigned I = 0, E = hdr->numMapEntries(); I != E; ++I) {
            FlashMapBlock block = map->blocks[I];
            if (block.isValid() && hdr->getEraseCount(eraseRef, block, I) <= averageEraseCount) {
                candidateVolumes.mark(index);
                break;
            }
        }
    }

    /*
     * If this set turns out to be empty for whatever reason
     * (say, we've already allocated all blocks with below-average
     * erase counts) we'll punt by making all deleted volumes into
     * candidates.
     */

    if (candidateVolumes.empty())
        candidateVolumes = deletedVolumes;
}

bool FlashBlockRecycler::next(FlashMapBlock &block, EraseCount &eraseCount)
{
    /*
     * We must start with orphaned blocks. See the explanation in the class
     * comment for FlashBlockRecycler. This part is easy- we assume they're
     * all at the average erase count.
     */

    unsigned index;
    if (orphanBlocks.clearFirst(index)) {
        block.setIndex(index);
        eraseCount = averageEraseCount;
        return true;
    }

    /*
     * Next, find a deleted volume to pull a block from. We use a 'candidateVolumes'
     * set in order to start working with volumes that contain blocks of a
     * below-average erase count.
     *
     * If we have a candidate volume at all, it's guaranteed to have at least
     * one recyclable block (the header).
     *
     * If we run out of viable candidate volumes, we can try to use
     * findCandidateVolumes() to refill the set. If that doesn't work,
     * we must conclude that the volume is full, and we return unsuccessfully.
     */

    FlashVolume vol;

    if (dirtyVolume.ref.isHeld()) {
        /*
         * We've already reclaimed some blocks from this deleted volume.
         * Keep working on the same volume, so we can reduce the number of
         * total writes to any volume's FlashMap.
         */
        vol = FlashMapBlock::fromAddress(dirtyVolume.ref->getAddress());
        ASSERT(vol.isValid());

    } else {
        /*
         * Use the next candidate volume, refilling the candidate set if
         * we need to.
         */

        unsigned index;
        if (!candidateVolumes.clearFirst(index)) {
            findCandidateVolumes();
            if (!candidateVolumes.clearFirst(index))
                return false;
        }
        vol = FlashMapBlock::fromIndex(index);
    }

    /*
     * Start picking arbitrary blocks from the volume. Since we're choosing
     * to wear-level only at the volume granularity (we process one deleted
     * volume at a time) it doesn't matter what order we pick them in, for
     * wear-levelling purposes at least.
     *
     * We do need to pick the volume header last, so start out iterating over
     * only non-header blocks.
     */

    FlashBlockRef ref;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, vol.block);
    FlashMap *map = hdr->getMap();

    for (unsigned I = 0, E = hdr->numMapEntries(); I != E; ++I) {
        FlashMapBlock candidate = map->blocks[I];
        if (candidate.isValid() && candidate.code != vol.block.code) {
            // Found a non-header block to yank! Mark it as dirty.

            dirtyVolume.beginBlock(ref);
            map->blocks[I].setInvalid();

            block = candidate;
            eraseCount = hdr->getEraseCount(ref, vol.block, I);
            return true;
        }
    }

    // Yanking the last (header) block. Remove this volume from the pool.
    
    vol.block.clear(deletedVolumes);
    dirtyVolume.commitBlock();

    block = vol.block;
    eraseCount = hdr->getEraseCount(ref, vol.block, 0);
    return true;
}

bool FlashVolumeWriter::begin(unsigned type, unsigned payloadBytes, unsigned hdrDataBytes)
{
    // Save the real type for later, it isn't written until commit()
    this->type = type;

    // Find one block to use for the volume header
    FlashBlockRecycler br;
    FlashMapBlock hdrBlock;
    FlashBlockRecycler::EraseCount hdrEC;
    if (!br.next(hdrBlock, hdrEC))
        return false;

    // Erase it
    hdrBlock.erase();
    hdrEC++;

    /*
     * Start building a FlashVolumeHeader in the cache.
     *
     * Initialize everything except the 'type' field.
     * This includes setting up map entries for the header block.
     */

    FlashBlockRef hdrRef;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(hdrRef, hdrBlock);

    unsigned payloadBlocks = (payloadBytes + FlashBlock::BLOCK_MASK) / FlashBlock::BLOCK_SIZE;
    FlashBlockWriter hdrWriter(hdrRef);
    ASSERT(hdrWriter.ref.isHeld());
    hdr->init(FlashVolume::T_INCOMPLETE, payloadBlocks, hdrDataBytes);

    FlashMap *map = hdr->getMap();
    map->blocks[0] = hdrBlock;

    FlashBlockWriter ecWriter;
    *ecWriter.getData<FlashBlockRecycler::EraseCount>
        (hdr->eraseCountAddress(hdrBlock, 0)) = hdrEC;

    /*
     * Start filling both the map and the erase count array.
     *
     * Note that we can fail to allocate a block at any point, but we
     * need to try our best to avoid losing any erase count data in the
     * event of an allocation failure or power loss.
     */

    bool success = true;

    for (unsigned I = 1, E = hdr->numMapEntries(); I < E; ++I) {
        FlashMapBlock block;
        FlashBlockRecycler::EraseCount ec;

        if (!br.next(block, ec)) {
            success = false;
            break;
        }

        block.erase();
        ec++;

        map->blocks[I] = block;
        *ecWriter.getData<FlashBlockRecycler::EraseCount>
            (hdr->eraseCountAddress(hdrBlock, I)) = ec;
    }

    /*
     * Regardless of whether we were successful or not, write a valid
     * set of CRCs to the header. Even if this volume remains incomplete,
     * we use these to validate the header when using it to recycle blocks.
     *
     * Note: We have a litle extra error checking for FlashBlockWriter here.
     * It is common for ecWriter and hdrWriter to actually both be referencing
     * the same block, so we can test that edge case here.
     */

    hdr->crcMap = hdr->calculateMapCRC();
    hdr->crcErase = hdr->calculateEraseCountCRC(hdrBlock);

    ASSERT(hdrWriter.ref.isHeld());
    ecWriter.commitBlock();
    ASSERT(hdrWriter.ref.isHeld());
    hdrWriter.commitBlock();
    ASSERT(!hdrWriter.ref.isHeld());

    /*
     * Initialize object members
     */

    payloadOffset = 0;
    volume = hdrBlock;
    ASSERT(volume.isValid());

    return success;
}

void FlashVolumeWriter::commit()
{
    // Finish writing the payload first, if necessary

    payloadWriter.commitBlock();

    // Just rewrite the header block, this time with the correct 'type'.

    FlashBlockRef ref;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, volume.block);

    FlashBlockWriter writer(ref);
    hdr->setType(type);
    writer.commitBlock();

    ASSERT(volume.isValid());
}

void FlashVolumeWriter::appendPayload(const uint8_t *bytes, uint32_t count)
{
    FlashBlockRef spanRef;
    FlashMapSpan span = volume.getPayload(spanRef);

    while (count) {
        uint32_t chunk = count;
        FlashBlockRef dataRef;
        FlashMapSpan::PhysAddr pa;
        unsigned flags = 0;

        if ((payloadOffset & FlashBlock::BLOCK_MASK) == 0) {
            // We know this is the beginning of a fully erased block
            flags |= FlashBlock::F_KNOWN_ERASED;
        }

        if (!span.getBytes(dataRef, payloadOffset, pa, chunk, flags)) {
            // This shouldn't happen unless we're writing past the end of the span!
            ASSERT(0);
            return;
        }

        payloadWriter.beginBlock(dataRef);
        memcpy(pa, bytes, chunk);

        count -= chunk;
        payloadOffset += chunk;
        bytes += chunk;
    }
}
