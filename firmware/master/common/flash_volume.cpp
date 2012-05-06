/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_volume.h"
#include "crc.h"


FlashVolume::Prefix *FlashVolume::getPrefix(FlashBlockRef &ref) const
{
    FlashBlock::get(ref, block.address());
    return reinterpret_cast<Prefix*>(ref->getData());
}

bool FlashVolume::Prefix::isValid() const
{
    /*
     * Validate everything we possibly can by looking only at the Prefix struct.
     */

    if (magic != MAGIC || flags != flagsCopy || type != typeCopy ||
        (uint16_t)numBlocks != (uint16_t)~numBlocksCpl)
        return false;

    if ((flags & F_HAS_MAP) == 0 && noMap.eraseCount != noMap.eraseCountCopy)
        return false;

    return true;
}

bool FlashVolume::isValid() const
{
    ASSERT(this);
    if (!block.isValid())
        return false;

    FlashBlockRef pRef;
    Prefix *p = getPrefix(pRef);
    
    if (!p->isValid())
        return false;

    // Check map CRCs
    if (p->flags & F_HAS_MAP) {
        FlashBlockRef mapRef;
        if (p->hasMap.crcMap != Crc32::object(*getMap(mapRef)))
            return false;
        for (unsigned i = 0; i < ERASE_COUNT_BLOCKS; i++)
            if (p->hasMap.crcErase[i] != Crc32::block(getEraseCountBlock(mapRef, i),
                ERASE_COUNTS_PER_BLOCK))
                return false;
    }

    return true;
}

const FlashMap *FlashVolume::getMap(FlashBlockRef &ref) const
{
    Prefix *p = getPrefix(ref);
    ASSERT(p->isValid());

    if (p->flags & F_HAS_MAP) {
        // Map is stored explicitly, immediately after the prefix
        STATIC_ASSERT(sizeof(FlashMap) + sizeof(Prefix) <= FlashBlock::BLOCK_SIZE);
        return reinterpret_cast<FlashMap*>(p + 1);

    } else {
        // We only have a single FlashMapBlock. There is no full FlashMap,
        // but we can treat our 'block' member as an immutable one-entry Map.
        return FlashMap::single(&block);
    }
}

unsigned FlashVolume::getOverheadBlockCount(unsigned flags)
{
    // How many blocks of overhead do we add prior to the volume data?

    unsigned blocks = 1;  // Prefix, type-specific data, map

    if (flags & F_HAS_MAP)
        blocks += ERASE_COUNT_BLOCKS;

    return blocks;
}

FlashMapSpan FlashVolume::getData(FlashBlockRef &ref) const
{
    Prefix *p = getPrefix(ref);
    ASSERT(p->isValid());
    unsigned flags = p->flags;
    unsigned numBlocks = p->numBlocks;
    unsigned overhead = getOverheadBlockCount(flags);

    return FlashMapSpan::create(getMap(ref), overhead, numBlocks - overhead);
}

unsigned FlashVolume::getType() const
{
    FlashBlockRef ref;
    Prefix *p = getPrefix(ref);
    ASSERT(p->isValid());
    return p->type;
}

unsigned FlashVolume::getNumBlocks() const
{
    FlashBlockRef ref;
    Prefix *p = getPrefix(ref);
    ASSERT(p->isValid());
    return p->numBlocks;
}

uint32_t *FlashVolume::getEraseCountBlock(FlashBlockRef &ref, unsigned index) const
{
    ASSERT(index < ERASE_COUNT_BLOCKS);
    FlashBlock::get(ref, block.address() + (index + 1) * FlashBlock::BLOCK_SIZE);
    return reinterpret_cast<uint32_t*>(ref->getData());
}

uint32_t FlashVolume::getEraseCount(unsigned index) const
{
    FlashBlockRef ref;
    Prefix *p = getPrefix(ref);

    if (p->flags & F_HAS_MAP) {
        ASSERT(index < FlashMap::NUM_MAP_BLOCKS);
        return getEraseCountBlock(ref, index / ERASE_COUNTS_PER_BLOCK)
            [index % ERASE_COUNTS_PER_BLOCK];

    } else {
        ASSERT(index == 0);
        return p->noMap.eraseCount;
    }
}

void FlashVolume::markAsDeleted() const
{
    FlashBlockRef ref;
    Prefix *p = getPrefix(ref);

    FlashBlockWriter writer(ref);
    p->type = T_DELETED;
    p->typeCopy = T_DELETED;
    writer.commit();
}

FlashVolume FlashVolumeIter::next()
{
    unsigned index;
    while (remaining.clearFirst(index)) {
        FlashVolume vol(FlashMapBlock::fromIndex(index));
        if (vol.isValid()) {

            // Don't visit any blocks that are part of this volume
            FlashBlockRef ref;
            vol.getMap(ref)->clear(remaining);

            return vol;
        }
    }

    return FlashMapBlock::invalid();
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
    while (FlashVolume v = vi.next()) {
        
        if (v.getType() == v.T_DELETED) {
            // Remember deleted volumes according to their header block
            v.getHeaderBlock().mark(deletedVolumes);
        }

        // If a block is reachable at all, even by a deleted volume, it isn't orphaned
        FlashBlockRef mapRef;
        const FlashMap *map = v.getMap(mapRef);
        map->clear(orphanBlocks);

        // Calculate the average erase count for all mapped blocks
        for (unsigned i = 0; i < arraysize(map->blocks); ++i) {
            FlashMapBlock block = map->blocks[i];
            if (block) {
                avgEraseNumerator += v.getEraseCount(i);
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
        FlashVolume v(FlashMapBlock::fromIndex(index));
        ASSERT(v.isValid());
        ASSERT(v.getType() == FlashVolume::T_DELETED);

        FlashBlockRef mapRef;
        const FlashMap *map = v.getMap(mapRef);

        for (unsigned i = 0; i < arraysize(map->blocks); ++i)
            if (map->blocks[i] && v.getEraseCount(i) <= averageEraseCount) {
                candidateVolumes.mark(index);
                break;
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

bool FlashBlockRecycler::next(FlashMapBlock &block, uint32_t &eraseCount)
{
    // Start with orphaned blocks. They're easy, they all use the average erase count.
    unsigned index;
    if (orphanBlocks.clearFirst(index)) {
        block.setIndex(index);
        eraseCount = averageEraseCount;
        return true;
    }

    // XXX: todo
    ASSERT(0);

    return false;
}
