/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_volume.h"


FlashVolume::Prefix *FlashVolume::getPrefix(FlashBlockRef &ref) const
{
    FlashBlock::get(ref, block.address());
    return reinterpret_cast<Prefix*>(ref->getData());
}

bool FlashVolume::Prefix::isValid() const
{
    return
        magic == MAGIC &&
        flags == flagsCopy &&
        type == typeCopy;
}

bool FlashVolume::isValid(FlashBlockRef &ref) const
{
    ASSERT(this);
    return getPrefix(ref)->isValid();
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

uint32_t FlashVolume::getEraseCount(unsigned index) const
{
    // XXX
    return 0;
}
