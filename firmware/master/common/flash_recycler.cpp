/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_volume.h"
#include "flash_volumeheader.h"
#include "flash_recycler.h"
#include "flash_eraselog.h"
#include "svmloader.h"


FlashBlockRecycler::FlashBlockRecycler(bool useEraseLog)
    : useEraseLog(useEraseLog)
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

    // Blocks in our Erase Log are not orphaned
    FlashEraseLog::clearBlocks(orphanBlocks);

    uint64_t avgEraseNumerator = 0;
    uint32_t avgEraseDenominator = 0;

    FlashVolumeIter vi;
    FlashVolume vol;

    vi.begin();
    while (vi.next(vol)) {

        FlashBlockRef ref;
        FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, vol.block);
        ASSERT(hdr->isHeaderValid());

        // Remember deleted or incomplete recyclable volumes according to their header block.
        // If the volume is still mapped by SVM, skip it for now. This allows in-use volumes
        // to be deleted, knowing that they won't be recycled until they are unmapped.

        if (FlashVolume::typeIsRecyclable(hdr->type) && !SvmLoader::isVolumeMapped(vol)) {
            vol.block.mark(deletedVolumes);
        }

        // If a block is reachable at all, even by a deleted volume, it isn't orphaned.
        // Also calculate the average erase count for all mapped blocks

        unsigned numMapEntries = hdr->numMapEntries();
        const FlashMap *map = hdr->getMap();
        FlashBlockRef eraseRef;

        for (unsigned I = 0; I != numMapEntries; ++I) {
            FlashMapBlock block = map->blocks[I];
            if (block.isValid()) {
                block.clear(orphanBlocks);
                avgEraseNumerator += hdr->getEraseCount(eraseRef, vol.block, I, numMapEntries);
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
     *
     * In order to avoid deleting the erase log unless we're really
     * low on space, we never include erase log volumes in this initial
     * list of candidates.
     */

    candidateVolumes.clear();

    FlashMapBlock::Set iterSet = deletedVolumes;
    unsigned index;
    while (iterSet.clearFirst(index)) {

        FlashBlockRef ref;
        FlashMapBlock block = FlashMapBlock::fromIndex(index);
        FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, block);

        if (hdr->type == FlashVolume::T_ERASE_LOG)
            continue;

        unsigned numMapEntries = hdr->numMapEntries();
        const FlashMap *map = hdr->getMap();
        FlashBlockRef eraseRef;

        ASSERT(FlashVolume(block).isValid());
        ASSERT(FlashVolume::typeIsRecyclable(hdr->type));
        ASSERT(hdr->isHeaderValid());

        for (unsigned I = 0; I != numMapEntries; ++I) {
            FlashMapBlock block = map->blocks[I];
            if (block.isValid() && hdr->getEraseCount(eraseRef, block, I, numMapEntries) <= averageEraseCount) {
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
     *
     * This also opens up T_ERASE_LOG volumes for recycling.
     */

    if (candidateVolumes.empty())
        candidateVolumes = deletedVolumes;
}

bool FlashBlockRecycler::next(FlashMapBlock &block, EraseCount &eraseCount)
{
    /*
     * If we're allowed to, see if we can take a shortcut by using the Erase Log.
     * This is a persistent FIFO which stores earlier output from this same
     * recycler algorithm, allowing us to pre-erase blocks whenever we have
     * time to kill, in order to avoid erase latency in the future.
     *
     * This needs to happen first, even before orphaned blocks. When the
     * filesystem is new (before every block has been erased once) there
     * will be a lot of orphaned blocks still. Those blocks need to be erased,
     * and we'd rather do that when we have time to spare.
     */

    if (useEraseLog) {
        FlashEraseLog::Record rec;
        if (eraseLog.pop(rec)) {
            block = rec.block;
            eraseCount = rec.ec;
            return true;
        }
    }

    /*
     * We must start with orphaned blocks. See the explanation in the class
     * comment for FlashBlockRecycler. This part is easy- we assume they're
     * all at the average erase count.
     */

    unsigned index;
    if (orphanBlocks.clearFirst(index)) {
        block.setIndex(index);
        block.erase();
        eraseCount = averageEraseCount + 1;
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
    unsigned numMapEntries = hdr->numMapEntries();
    FlashMap *map = hdr->getMap();

    for (unsigned I = 0; I != numMapEntries; ++I) {
        FlashMapBlock candidate = map->blocks[I];
        if (candidate.isValid() && candidate.code != vol.block.code) {
            // Found a non-header block to yank! Mark it as dirty.

            dirtyVolume.beginBlock(ref);
            map->blocks[I].setInvalid();

            block = candidate;
            block.erase();
            eraseCount = 1 + hdr->getEraseCount(ref, vol.block, I, numMapEntries);
            return true;
        }
    }

    // Yanking the last (header) block. Remove this volume from the pool.
    
    vol.block.clear(deletedVolumes);
    dirtyVolume.commitBlock();

    block = vol.block;
    block.erase();
    eraseCount = 1 + hdr->getEraseCount(ref, vol.block, 0, numMapEntries);
    return true;
}
