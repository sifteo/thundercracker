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
    FlashBlockRef ref;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, block);
    ASSERT(hdr->isHeaderValid());
    return hdr->type;
}

FlashMapSpan FlashVolume::getData(FlashBlockRef &ref) const
{
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
    writer.commit();
}

bool FlashVolumeIter::next(FlashVolume &vol)
{
    unsigned index;
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
                if (block)
                    block.clear(remaining);
            }

            return true;
        }
    }

    return false;
}

FlashBlockRecycler::FlashBlockRecycler()
    : dirtyVolume(FlashMapBlock::invalid())
{
    dirtyBlocks.clear();
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
    while (vi.next(vol)) {

        FlashBlockRef ref;
        FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, vol.block);
        ASSERT(hdr->isHeaderValid());

        if (hdr->type == FlashVolume::T_DELETED) {
            // Remember deleted volumes according to their header block
            vol.block.mark(deletedVolumes);
        }

        // If a block is reachable at all, even by a deleted volume, it isn't orphaned.
        // Also calculate the average erase count for all mapped blocks

        const FlashMap *map = hdr->getMap();
        FlashBlockRef eraseRef;

        for (unsigned I = 0, E = hdr->numMapEntries(); I != E; ++I) {
            FlashMapBlock block = map->blocks[I];
            if (block) {
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
        ASSERT(hdr->type == FlashVolume::T_DELETED);
        ASSERT(hdr->isHeaderValid());

        for (unsigned I = 0, E = hdr->numMapEntries(); I != E; ++I) {
            FlashMapBlock block = map->blocks[I];
            if (block && hdr->getEraseCount(eraseRef, block, I) <= averageEraseCount) {
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

void FlashBlockRecycler::commit()
{
    /*
     * Invalidate some set of dirty blocks in a volume's map.
     * If we have no currently dirty blocks, this has no effect.
     */

    if (!dirtyVolume.block)
        return;

    if (dirtyBlocks.empty())
        return;

    FlashBlockRef ref;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, dirtyVolume.block);
    FlashMap *map = hdr->getMap();
    FlashBlockWriter writer(ref);

    unsigned index;
    while (dirtyBlocks.clearFirst(index)) {
        ASSERT(index < hdr->numMapEntries());
        map->blocks[index].setInvalid();
    }

    writer.commit();
    dirtyVolume = FlashMapBlock::invalid();
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

    if (dirtyVolume.block) {
        /*
         * We've already reclaimed some blocks from this deleted volume.
         * Keep working on the same volume, so we can reduce the number of
         * total writes to any volume's FlashMap.
         */
        vol = dirtyVolume;

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
        if (candidate && candidate != vol.block) {
            // Found a non-header block to yank! Mark it as dirty, commit it later.

            if (vol.block != dirtyVolume.block) {
                commit();
                dirtyVolume = vol;
            }

            candidate.mark(dirtyBlocks);
            block = candidate;
            eraseCount = hdr->getEraseCount(ref, vol.block, I);
            return true;
        }
    }

    // Yanking the last (header) block.
    commit();
    block = vol.block;
    eraseCount = hdr->getEraseCount(ref, vol.block, 0);
    return true;
}
