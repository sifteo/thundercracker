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

    if (magic != MAGIC || flags != flagsCopy || type != typeCopy)
        return false;

    if ((flags & F_HAS_MAP) == 0 && noMap.eraseCount != noMap.eraseCountCopy)
        return false;

    return true;
}

bool FlashVolume::isValid() const
{
    ASSERT(this);
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
