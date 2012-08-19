/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_syslfs.h"
#include "crc.h"
#include "bits.h"
#include "prng.h"
#include "cube.h"
#include "cubeslots.h"


int SysLFS::read(Key k, uint8_t *buffer, unsigned bufferSize)
{
    // System internal version of _SYS_fs_objectRead()
    
    ASSERT(FlashLFSIndexRecord::isKeyAllowed(k));

    FlashLFS &lfs = SysLFS::get();
    FlashLFSObjectIter iter(lfs);

    while (iter.previous(FlashLFSKeyQuery(k))) {
        unsigned size = iter.record()->getSizeInBytes();
        size = MIN(size, bufferSize);
        if (iter.readAndCheck(buffer, size))
            return size;
    }

    return _SYS_ENOENT;
}

int SysLFS::write(Key k, const uint8_t *data, unsigned dataSize, bool gc)
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

    if (gc) {
        // Normal allocation; allow garbage collection
        
        if (!allocator.allocateAndCollectGarbage()) {
            // We don't expect callers to have a good way to cope with this
            // failure, so go ahead and log the error early.
            LOG(("SYSLFS: Out of space, failed to write system data to flash!\n"));
            return _SYS_ENOSPC;
        }
    } else {
        /*
         * The caller has specifically requested to disable GC.
         *
         * This can be used if time is tight, or if the caller is relying
         * on existing LFS volumes not to be deleted, for example if the
         * caller is also iterating over existing records.
         *
         * We also assume here that the caller has a specific way to handle
         * out-of-space errors, so don't log anything if we fail here.
         */
        if (!allocator.allocate())
            return _SYS_ENOSPC;
    }

    FlashBlock::invalidate(allocator.address(), allocator.address() + dataSize);
    FlashDevice::write(allocator.address(), data, dataSize);
    return dataSize;
}

SysLFS::Key SysLFS::CubeRecord::makeKey(_SYSCubeID cube)
{
    // CubeSlots store their own mapping back to their paired CubeRecord key
    ASSERT(cube < _SYS_NUM_CUBE_SLOTS);
    return CubeSlots::instances[cube].getCubeRecordKey();
}

bool SysLFS::CubeRecord::decodeKey(Key cubeKey, _SYSCubeID &cube)
{
    /*
     * Reverse mapping from key back to Cube ID. Since only
     * connected cubes have an associated CubeRecord, this only
     * searches the vector of sysConnected cubes.
     */

    _SYSCubeIDVector cv = CubeSlots::sysConnected;
    while (cv) {
        _SYSCubeID c = Intrinsic::CLZ(cv);
        cv ^= Intrinsic::LZ(c);

        if (makeKey(c) == cubeKey) {
            cube = c;
            return true;
        }
    }

    return false;
}

void SysLFS::CubeRecord::init()
{
    memset(this, 0, sizeof *this);
}

bool SysLFS::CubeRecord::load(const FlashLFSObjectIter &iter)
{
    unsigned size = iter.record()->getSizeInBytes();

    if (size == 0) {
        // Deleted record.
        init();
        return true;
    }

    // Valid if CRC is okay
    return size >= sizeof *this && iter.readAndCheck((uint8_t*) this, sizeof *this);
}

void SysLFS::PairingIDRecord::init()
{
    // 0xFF is always an invalid HWID byte
    memset(this, 0xFF, sizeof *this);
}

void SysLFS::PairingIDRecord::load()
{
    if (!read(SysLFS::kPairingID, *this))
        init();
}

void SysLFS::PairingMRURecord::init()
{
    // Fill with invalid indices. We'll fix it up in load().
    memset(this, 0xFF, sizeof *this);
}

void SysLFS::PairingMRURecord::load()
{
    if (!read(SysLFS::kPairingMRU, *this))
        init();

    /*
     * We need the MRU list to contain exactly one copy of each index,
     * so we're sure to use the full capacity of the pairing table
     * when assigning new IDs. So, at load-time, we will scan through
     * the record and replace any duplicate or invalid IDs with unrepresented IDs.
     *
     * This avoids logic errors during pairing, if we had some other data
     * corruption issue which leads to a badly formed MRU record. For example,
     * if we were to write the MRU list to flash while the ISR is updating it.
     * These cases are rare, and it isn't worth our while to use heavier synchronization
     * primitives- this is just a caching hint anyway, so it's fine as long as we don't
     * end up with a long-term problem.
     */

    // First pass: Which indices are missing?

    BitVector<NUM_PAIRINGS> missing;
    missing.mark();

    for (unsigned i = 0; i < NUM_PAIRINGS; ++i) {
        unsigned r = rank[i];
        if (r < NUM_PAIRINGS)
            missing.clear(r);
    }

    // Second pass: Look for duplicated/invalid elements, fill in with missing ones

    BitVector<NUM_PAIRINGS> found;
    found.clear();

    for (unsigned i = 0; i < NUM_PAIRINGS; ++i) {
        if (rank[i] >= NUM_PAIRINGS || found.test(rank[i])) {
            unsigned recycled = 0;
            ASSERT(!missing.empty());
            missing.clearFirst(recycled);
            rank[i] = recycled;
        }
        found.mark(rank[i]);
    }

    ASSERT(missing.empty());
}

bool SysLFS::PairingMRURecord::access(unsigned index)
{
    /*
     * Move 'index' to the front of the list.
     * Returns 'true' if this entailed making any changes, 'false' if not.
     */

    ASSERT(index < NUM_PAIRINGS);

    if (rank[0] == index)
        return false;

    unsigned i;
    for (i = 0; i < NUM_PAIRINGS; ++i) {
        if (rank[i] == index)
            break;
    }
    if (i == NUM_PAIRINGS) {
        /*
         * Not in list. Shouldn't happen unless our list is inconsistent,
         * but this can happen. See load() above. In that case, we assume
         * we're replacing the last element.
         */
        i--;
    }

    // Make room. Shift all prior elements up by one.
    ASSERT(i > 0);
    memmove(&rank[1], &rank[0], sizeof(rank[0]) * i);

    rank[0] = index;
    return true;
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
    for (unsigned slot = 0; slot < arraysize(slots); ++slot) {
        AssetSlotOverviewRecord &ovr = slots[slot];
        if (ovr.identity.volume == vol.block.code)
            ovr.identity.volume = 0;
    }

    /*
     * Pick a bank, by analyzing the relative cost of using one bank or the other.
     * We try a mock allocation using both banks, saving whichever one is lower cost.
     *
     * Note the "<=" cost check below. This ensures that, in the event of a tie,
     * we start with the last bank in the tie. This slight preference toward
     * nonzero banks helps us ensure that the bank switching code paths
     * get well-exercised.
     */

    // Assuming evenly sized banks
    STATIC_ASSERT((ASSET_SLOTS_PER_CUBE % ASSET_SLOTS_PER_BANK) == 0);

    unsigned bestCost = unsigned(-1);
    SlotVector_t bestVec;

    for (unsigned bank = 0; bank < arraysize(slots); bank += ASSET_SLOTS_PER_BANK) {
        SlotVector_t vec;
        unsigned cost;
        recycleSlots(bank, numSlots, vec, cost);
        if (cost <= bestCost) {
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
     * Increase the erase count for a specific slot, and reset
     * the number of allocated tiles to zero.
     *
     * Always modifies the record.
     */
     
    AssetSlotOverviewRecord &s = slots[slot];

    // Assuming our erase counts are 8-bit.
    STATIC_ASSERT(sizeof s.eraseCount == 1);

    if (s.eraseCount == 0xFF) {
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

    s.eraseCount++;
    ASSERT(s.eraseCount > 0);
}

bool SysLFS::CubeAssetsRecord::markAccessed(FlashVolume vol, unsigned numSlots, bool force)
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
     *
     * If 'force' is true, we disregard the slots' existing access ranks.
     */

    bool modified = false;

    for (unsigned i = 0; i < arraysize(slots); ++i) {
        AssetSlotOverviewRecord &slot = slots[i];
        if (slot.identity.inActiveSet(vol, numSlots)) {
            if (force || slot.accessRank != 0) {
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

SysLFS::Key SysLFS::AssetSlotRecord::makeKey(Key cubeKey, unsigned slot)
{
    unsigned i = cubeKey - kCubeBase;
    ASSERT(i < kCubeCount);
    ASSERT(slot < ASSET_SLOTS_PER_CUBE);
    return Key(kAssetSlotBase + i * ASSET_SLOTS_PER_CUBE + slot);
}

bool SysLFS::AssetSlotRecord::decodeKey(Key slotKey, Key &cubeKey, unsigned &slot)
{
    unsigned i = slotKey - kAssetSlotBase;
    if (i < kAssetSlotCount) {
        cubeKey = (Key) ((i / ASSET_SLOTS_PER_CUBE) + kCubeBase);
        slot = i % ASSET_SLOTS_PER_CUBE;
        return true;
    }
    return false;
}

void SysLFS::AssetSlotRecord::init()
{
    flags = 0;
    memset(crc, 0x00, sizeof crc);
    memset(groups, 0xff, sizeof groups);
}

bool SysLFS::AssetSlotRecord::load(const FlashLFSObjectIter &iter)
{
    unsigned size = iter.record()->getSizeInBytes();

    // Default contents
    init();

    // Variable-sized data portion, valid if the CRC is okay.
    return iter.readAndCheck((uint8_t*) this, MIN(size, sizeof *this));
}

unsigned SysLFS::AssetSlotRecord::writeableSize() const
{
    // How many bytes do we need to write for this record?

    STATIC_ASSERT(sizeof flags + sizeof crc + sizeof groups == sizeof *this);
    return totalGroups() * sizeof groups[0] + sizeof flags + sizeof crc;
}

bool SysLFS::AssetSlotRecord::findGroup(AssetGroupIdentity identity, unsigned &offset) const
{
    /*
     * Look for a matching group in this slot record. We keep searching
     * until we reach an empty record. Also, if a load is or was
     * in-progress, we refuse to trust the last group in the record.
     */

    ASSERT(identity.volume != 0);

    unsigned inProgress = flags & F_LOAD_IN_PROGRESS;
    unsigned currentOffset = 0;

    for (unsigned i = 0; i < ASSET_GROUPS_PER_SLOT; ++i) {
        const LoadedAssetGroupRecord &group = groups[i];
    
        if (group.isEmpty())
            break;
        if (inProgress && (i + 1) < ASSET_GROUPS_PER_SLOT && groups[i + 1].isEmpty())
            break;

        if (group.identity == identity) {
            offset = currentOffset;
            return true;
        }

        currentOffset += group.size.tileCount();
    }

    return false;
}

bool SysLFS::AssetSlotRecord::allocGroup(AssetGroupIdentity identity,
    unsigned numTiles, unsigned &offset)
{
    /*
     * Append a record for the given group identity. On success, writes
     * the group's load address offset, in tiles, to "offset" and returns true.
     *
     * On allocation failure (No more group slots, not enough free tiles)
     * return false without modifying the AssetSlotRecord.
     */

    // Refuse to allocate if a load was in progress. The erasure state is indeterminate.
    if (flags & F_LOAD_IN_PROGRESS)
        return false;

    unsigned currentOffset = 0;

    for (unsigned i = 0; i < ASSET_GROUPS_PER_SLOT; ++i) {
        LoadedAssetGroupRecord &group = groups[i];
    
        if (group.isEmpty()) {
            // Found a spot to allocate at!

            if (currentOffset + numTiles > SysLFS::TILES_PER_ASSET_SLOT) {
                // Slot is full!
                return false;
            }

            group.size = AssetGroupSize::fromTileCount(numTiles);
            group.identity = identity;
            offset = currentOffset;
            return true;
        }

        currentOffset += group.size.tileCount();
    }

    // Not enough free group records!
    return false;
}

unsigned SysLFS::AssetSlotRecord::totalTiles() const
{
    /*
     * How much space is used in this slot, measuring in tiles?
     */

    // Refuse to allocate if a load was in progress.
    if (flags & F_LOAD_IN_PROGRESS)
        return TILES_PER_ASSET_SLOT;

    unsigned currentOffset = 0;

    for (unsigned i = 0; i < ASSET_GROUPS_PER_SLOT; ++i) {
        const LoadedAssetGroupRecord &group = groups[i];
        if (group.isEmpty())
            break;
        currentOffset += group.size.tileCount();
    }

    if (currentOffset > TILES_PER_ASSET_SLOT) {
        // Shouldn't happen... this means the slot is overfull.
        ASSERT(0);
        return 0;
    }

    return currentOffset;
}

unsigned SysLFS::AssetSlotRecord::totalGroups() const
{
    /*
     * How many groups are in use, in this slot?
     */

    unsigned i;
    for (i = 0; i < ASSET_GROUPS_PER_SLOT; ++i) {
        const LoadedAssetGroupRecord &group = groups[i];
        if (group.isEmpty())
            break;
    }

    return i;
}

bool SysLFS::AssetSlotRecord::isEmpty() const
{
    return tilesFree() == TILES_PER_ASSET_SLOT;
}

void SysLFS::deleteAll()
{
    FlashVolumeIter vi;
    FlashVolume vol;

    vi.begin();
    while (vi.next(vol)) {
        if (vol.getType() == FlashVolume::T_LFS && !vol.getParent().block.isValid())
            vol.deleteSingle();
    }
}

void SysLFS::deleteCube(unsigned index)
{
    /*
     * Delete all saved information about a particular cube index
     */

    ASSERT(index < kCubeCount);

    Key cubeKey = Key(kCubeBase + index);
    write(cubeKey, 0, 0);

    for (unsigned i = 0; i < ASSET_SLOTS_PER_CUBE; ++i)
        write(AssetSlotRecord::makeKey(cubeKey, i), 0, 0);
}

void SysLFS::cleanupDeletedVolumes()
{
    /*
     * Scrub through SysLFS, and remove any referencs to volumes which are
     * deleted and no longer mapped. We do this just before creating a new
     * game or launcher volume.
     *
     * This is how we remove references to AssetGroups in games which have
     * been deleted, for example. There are a couple constraints that
     * make this process a bit delicate:
     *
     *   - We can't do this from a place where we'd create a re-entrancy
     *     hazard, such as during block recycling. (We may need to recycle
     *     blocks while writing new SysLFS records, and having two block
     *     recyclers active at once is a very bad thing.)
     *
     *   - We need to be kind to volumes which have been deleted but are
     *     still running. Ideally these unlinked volumes can still make use
     *     of cached assets until they finally stop running.
     *
     * So, this lazy process parallels the way blocks are recycled. We only
     * clean up stale volume references when we may be about to re-use those
     * same block codes for other volumes.
     *
     * Places where we need to check volume references:
     *
     *   - AssetSlotIdentity, inside the CubeRecord
     *   - AssetGroupIdentity, inside the AssetSlotRecord
     *
     * Each individual record type has a cleanupDeletedVolumes() function
     * which does the actual work. This function manages the overall SysLFS
     * iteration loop.
     */

    /*
     * Iterate over volumes once, to build a map of which volumes still exist.
     */

    FlashMapBlock::Set allVolumes;
    allVolumes.clear();
    {
        FlashVolumeIter vi;
        FlashVolume vol;
        vi.begin();
        while (vi.next(vol))
            vol.block.mark(allVolumes);
    }

    /*
     * Restartable iteration over all of SysLFS
     */

    // Exclude anything below kCubeBase. As we go, we'll exclude already-visited keys.
    FlashLFSIndexRecord::KeyVector_t excluded;
    excluded.clear();
    for (unsigned i = 0; i < SysLFS::kCubeBase; ++i)
        excluded.mark(i);

    FlashLFS &lfs = get();
    while (1) {
        FlashLFSObjectIter iter(lfs);
        while (1) {
            
            if (!iter.previous(FlashLFSKeyQuery(&excluded))) {
                // Out of records; done
                return;
            }

            // Is this an overview record?
            _SYSCubeID cube;
            SysLFS::Key key = (SysLFS::Key) iter.record()->getKey();
            if (SysLFS::CubeRecord::decodeKey(key, cube)) {

                // Load it. On success, exclude key from future iteration.
                SysLFS::CubeRecord cr;
                if (!cr.load(iter))
                    continue;
                excluded.mark(key);

                if (cr.cleanupDeletedVolumes(allVolumes)) {
                    if (SysLFS::write(key, cr, false))
                        continue;
                    // Enable GC, and restart iteration.
                    SysLFS::write(key, cr);
                    break;
                }
            }

            // Is this an AssetSlotRecord?
            SysLFS::Key cubeKey;
            unsigned slot;
            if (SysLFS::AssetSlotRecord::decodeKey(key, cubeKey, slot)) {

                // Load it. On success, exclude key from future iteration.
                SysLFS::AssetSlotRecord asr;
                if (!asr.load(iter))
                    continue;
                excluded.mark(key);

                if (asr.cleanupDeletedVolumes(allVolumes)) {
                    if (SysLFS::write(key, asr, false))
                        continue;
                    // Enable GC, and restart iteration.
                    SysLFS::write(key, asr);
                    break;
                }
            }
        }
    }
}

bool SysLFS::CubeRecord::cleanupDeletedVolumes(const FlashMapBlock::Set &allVolumes)
{
    /*
     * Delete any references to deleted volumes within this CubeRecord,
     * given a vector of volumes which are still valid.
     *
     * Returns 'true' if any changes were made, or 'false' if none were needed.
     */

    bool changed = false;

    for (unsigned slot = 0; slot < ASSET_SLOTS_PER_CUBE; ++slot) {
        SysLFS::AssetSlotIdentity &id = assets.slots[slot].identity;
        FlashMapBlock vol = FlashMapBlock::fromCode(id.volume);

        if (vol.isValid() && !vol.test(allVolumes)) {
            // Found a deleted volume. Erase the slot.

            memset(&id, 0, sizeof id);
            assets.markErased(slot);
            changed = true;
        }
    }

    return changed;
}

bool SysLFS::AssetSlotRecord::cleanupDeletedVolumes(const FlashMapBlock::Set &allVolumes)
{
    /*
     * Delete any references to deleted volumes within this AssetSlotRecord,
     * given a vector of volumes which are still valid.
     *
     * Returns 'true' if any changes were made, or 'false' if none were needed.
     */

    bool changed = false;

    for (unsigned group = 0; group < ASSET_GROUPS_PER_SLOT; ++group) {
        SysLFS::AssetGroupIdentity &id = groups[group].identity;
        FlashMapBlock vol = FlashMapBlock::fromCode(id.volume);

        if (vol.isValid() && !vol.test(allVolumes)) {
            // Found a deleted volume. Invalidate its volume code, but leave the rest of the slot intact.
            // This will prevent the deleted group from being used, but other groups will still be usable.

            memset(&id, 0, sizeof id);
            changed = true;
        }
    }

    return changed;
}
