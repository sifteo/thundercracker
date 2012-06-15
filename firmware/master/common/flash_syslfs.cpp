/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_syslfs.h"
#include "crc.h"
#include "bits.h"
#include "prng.h"


int SysLFS::read(Key k, uint8_t *buffer, unsigned bufferSize)
{
    // System internal version of _SYS_fs_objectRead()
    
    ASSERT(FlashLFSIndexRecord::isKeyAllowed(k));

    FlashLFS &lfs = SysLFS::get();
    FlashLFSObjectIter iter(lfs);

    while (iter.previous(k)) {
        unsigned size = iter.record()->getSizeInBytes();
        size = MIN(size, bufferSize);
        if (iter.readAndCheck(buffer, size))
            return size;
    }

    return _SYS_ENOENT;
}

int SysLFS::write(Key k, const uint8_t *data, unsigned dataSize)
{
    // System internal version of _SYS_fs_objectWrite()

    ASSERT(FlashLFSIndexRecord::isKeyAllowed(k));
    ASSERT(FlashLFSIndexRecord::isSizeAllowed(dataSize));

    CrcStream cs;
    cs.reset();
    cs.addBytes(data, dataSize);
    uint32_t crc = cs.get(FlashLFSIndexRecord::SIZE_UNIT);

    FlashLFS &lfs = SysLFS::get();
    FlashLFSObjectAllocator allocator(lfs, k, dataSize, crc);

    if (!allocator.allocateAndCollectGarbage()) {
        // We don't expect callers to have a good way to cope with this
        // failure, so go ahead and log the error early.
        LOG(("SYSLFS: Out of space, failed to write system data to flash!\n"));
        return _SYS_ENOSPC;
    }

    FlashBlock::invalidate(allocator.address(), allocator.address() + dataSize);
    FlashDevice::write(allocator.address(), data, dataSize);
    return dataSize;
}

SysLFS::Key SysLFS::CubeRecord::getByCubeID(_SYSCubeID cube)
{
    /*
     * TO DO: The association from _SYSCubeID to SysLFS::Key should be
     *        made at the time of pairing, and probably stored by CubeSlot.
     *        When the cube is first pair, we either find an existing
     *        CubeRecord for that HWID, or we allocate a new CubeRecord by
     *        recycling a 'dead' cube's Key.
     *
     *        Until pairing is in place, we use a 1:1 mapping between CubeID
     *        and Key.
     */

    ASSERT(cube < _SYS_NUM_CUBE_SLOTS);
    Key k = Key(kCubeBase + cube);

    if (!SysLFS::read(k, *this)) {
        // Initialize with default contents
        memset(this, 0, sizeof *this);
    }

    return k;
}

bool SysLFS::CubeAssetsRecord::checkBinding(FlashVolume vol, unsigned numSlots) const
{
    /*
     * Returns 'true' if the record has all ordinals 0 through numSlots-1
     * for the indicated volume. If any slots are missing, or slots are
     * allocated in different banks, returns false.
     */

    // Bits are set when we discover a match
    SlotVector_t ordinals;
    ordinals.clear();

    // Required bit mask
    uint32_t mask = 0xFFFFFFFF << (32 - numSlots);
    
    // Assuming evenly sized banks
    STATIC_ASSERT((ASSET_SLOTS_PER_CUBE % ASSET_SLOTS_PER_BANK) == 0);

    for (unsigned slot = 0; slot < arraysize(slots); ++slot) {
        if ((slot % ASSET_SLOTS_PER_BANK) == 0) {
            /*
             * Just before the first slot in a bank. Either nothing has been allocated,
             * or everything has. Any other state means we're straddling banks.
             */
            if (ordinals.empty()) {
                // This is okay. Nothing found, time to search the next bank.
            } else if ((ordinals.words[0] & mask) == mask) {
                // Found a good match in the first bank!
                return true;
            } else {
                // Partial match! No good.
                return false;
            }
        }

        const AssetSlotIdentity &id = slots[slot].identity;

        if (id.volume == vol.block.code) {
            unsigned ordinal = id.ordinal;
            if (ordinal >= ASSET_SLOTS_PER_CUBE) {
                // Bad ordinal value. No good!
                return false;
            }
            if (ordinals.test(ordinal)) {
                // Duplicate ordinal. No good!
                return false;
            }
            ordinals.mark(ordinal);
        }
    }

    // Found a good match in the last bank?
    return (ordinals.words[0] & mask) == mask;
}

void SysLFS::CubeAssetsRecord::allocBinding(FlashVolume vol, unsigned numSlots)
{
    /*
     * Allocate ordinals [0, numSlots-1] for the specified volume, in this
     * CubeAssetsRecord. Any existing allocations for the same volume will
     * be discarded, and we may choose to recycle any other slots as well.
     *
     * Note that this is the only point at which we do wear-leveling or manage
     * eviction strategy for cube flash!
     */

    // Should only be invoked if there's no existing vaild binding
    ASSERT(!checkBinding(vol, numSlots));

    // Zap all existing bindings for this volume
    for (unsigned slot = 0; slot < arraysize(slots); ++slot)
        slots[slot].identity.volume = 0;

    /*
     * Pick a bank, by analyzing the relative cost of using one bank or the other.
     * We try a mock allocation using both banks, saving whichever one is lower cost.
     */

    // Assuming evenly sized banks
    STATIC_ASSERT((ASSET_SLOTS_PER_CUBE % ASSET_SLOTS_PER_BANK) == 0);

    unsigned bestCost = unsigned(-1);
    SlotVector_t bestVec;

    for (unsigned bank = 0; bank < arraysize(slots); bank += ASSET_SLOTS_PER_BANK) {
        SlotVector_t vec;
        unsigned cost;
        recycleSlots(bank, numSlots, vec, cost);
        if (cost < bestCost) {
            bestCost = cost;
            bestVec = vec;
        }
    }

    /*
     * Lock in the selection. The order in which we map 'bestVec' to ordinals
     * is arbitrary. In order to avoid any systemic bias in flash memory wear
     * due to specific ordinals being more frequently erased than others,
     * we randomize the assignment of ordinals at this time.
     *
     * This shuffling algorithm randomly chooses to extract either the
     * first or the last bit of the SlotVector.
     */

    // We assume SlotVector is one machine word, so we have enough entropy.
    STATIC_ASSERT(sizeof bestVec.words == 4);
    uint32_t entropy = PRNG::anonymousValue();

    for (unsigned ordinal = 0; ordinal < numSlots; ordinal++) {
        ASSERT(!bestVec.empty());

        // Flip a coin and take either the first or last slot.
        unsigned index = (entropy & 1)
            ? Intrinsic::CLZ(bestVec.words[0])
            : (31 - Intrinsic::CTZ(bestVec.words[0]));
        entropy >>= 1;

        ASSERT(bestVec.test(index));
        bestVec.clear(index);
        AssetSlotOverviewRecord &slot = slots[index];

        // Start out empty, but preserve erase count and access rank.
        slot.numAllocatedTiles = 0;
        slot.identity.volume = vol.block.code;
        slot.identity.ordinal = ordinal;
    }

    // recycleSlots() should have given us exactly the right number of slots.
    ASSERT(bestVec.empty());

    // We're done. This should count as a valid binding now!
    ASSERT(checkBinding(vol, numSlots));
}

unsigned SysLFS::AssetSlotOverviewRecord::costToEvict() const
{
    /*
     * How bad is it to evict this record from flash, necessitating
     * a re-binding for whatever volume owned this slot? Smaller numbers
     * make a slot more likely to get recycled.
     */

    if (identity.volume == 0) {
        // Unclaimed, free to evict
        return 0;
    }

    /*
     * This function sets the way we weight accessRank vs. eraseCount
     * when deciding what to evict next. This places the most weight
     * on preserving recently-accessed data, but that can always be
     * superceded by a difference in eraseCount of more than 128, so
     * that we can perform some amount of static wear leveling too.
     * Slots that were accessed less frequently (higher accessRank)
     * require an even smaller difference in eraseCount.
     */

    unsigned cost = eraseCount + 0x80 / (1 + accessRank);
    return MIN(cost, MAX_COST);
}

void SysLFS::CubeAssetsRecord::recycleSlots(unsigned bank, unsigned numSlots,
    SlotVector_t &vecOut, unsigned &costOut) const
{
    /*
     * Choose the best way to allocate 'numSlots' total slots within the bank
     * that begins at slot 'bank'. Writes both our solution (the set of slots
     * to use) and the cost of that solution (with a lower-is-better metric)
     * to the provided output parameters.
     *
     * To limit the algorithmic complexity of this operation, we operate
     * on each slot independently, without taking into account that evicting
     * a slot also means that any other slots belonging to the same volume
     * will need to be rebound.
     *
     * Our N is so small here that we don't have to worry much about
     * sorting. Just do a simple selection sort.
     */
     
    ASSERT((bank % ASSET_SLOTS_PER_BANK) == 0);
    ASSERT(numSlots <= ASSET_SLOTS_PER_BANK);

    // Calculate all costs only once

    uint8_t costs[ASSET_SLOTS_PER_BANK];
    for (unsigned i = 0; i < arraysize(costs); ++i)
        costs[i] = slots[bank + i].costToEvict();

    // Select the next best slot until we've met our quota.

    unsigned totalCost = 0;
    unsigned count = 0;
    SlotVector_t vec;
    vec.clear();

    while (count < numSlots) {
        unsigned best = 0;
        for (unsigned i = 1; i < arraysize(costs); ++i) {
            if (costs[i] < costs[best])
                best = i;
        }

        ASSERT(vec.test(bank + best) == false);
        vec.mark(bank + best);
        totalCost += costs[best];

        STATIC_ASSERT(AssetSlotOverviewRecord::MAX_COST + 1 < 0x100);
        costs[best] = AssetSlotOverviewRecord::MAX_COST + 1;
        count++;
    }

    vecOut = vec;
    costOut = totalCost;
}

void SysLFS::CubeAssetsRecord::markErased(unsigned slot)
{
    /*
     * Increase the erase count for a specific slot.
     * Always modifies the record.
     */

    // Assuming our erase counts are 8-bit.
    STATIC_ASSERT(sizeof slots[slot].eraseCount == 1);

    if (slots[slot].eraseCount == 0xFF) {
        /*
         * If our erase counts are about to overflow, we get them back in
         * range by (1) subtracting the largest constant offset we can, or
         * if necessary by (2) dividing them in half.
         */

        unsigned minEraseCount = unsigned(-1);
        for (unsigned i = 0; i < arraysize(slots); ++i)
            minEraseCount = MIN(minEraseCount, slots[i].eraseCount);

        for (unsigned i = 0; i < arraysize(slots); ++i) {
            if (minEraseCount)
                slots[i].eraseCount -= minEraseCount;
            else
                slots[i].eraseCount >>= 1;
        }
    }

    slots[slot].eraseCount++;
    ASSERT(slots[slot].eraseCount > 0);
}

bool SysLFS::CubeAssetsRecord::markAccessed(FlashVolume vol, unsigned numSlots)
{
    /*
     * Mark a volume's slots as having been recently accessed. This
     * sets the accessRank of the slots in question to zero, and increments
     * all other slots' ranks.
     *
     * Returns 'true' if the record was modified at all. If the slots in
     * question were already at rank 0, nothing needs to be written.
     *
     * Note that this intentionally leaves 'holes' in the ranking when we
     * bump a slot back to rank zero. That's fine. It's also important that
     * slots belonging to the same volume all get ranked equivalently, since
     * there's no reason to prefer one slot to another except for wear leveling.
     */

    bool modified = false;

    for (unsigned i = 0; i < arraysize(slots); ++i) {
        AssetSlotOverviewRecord &slot = slots[i];
        if (slot.identity.inActiveSet(vol, numSlots)) {
            if (slot.accessRank != 0) {
                modified = true;
                slot.accessRank = 0;
            }
        }
    }

    // Only update other ranks if there has been a change
    if (modified) {
        for (unsigned i = 0; i < arraysize(slots); ++i) {
            AssetSlotOverviewRecord &slot = slots[i];
            if (!slot.identity.inActiveSet(vol, numSlots)
                && slot.accessRank < 0xFF) {
                slot.accessRank++;
            }
        }
    }

    return modified;
}
