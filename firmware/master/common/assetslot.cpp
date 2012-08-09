/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "assetslot.h"
#include "assetutil.h"
#include "cube.h"
#include "cubeslots.h"
#include "machine.h"
#include "svmruntime.h"
#include "svmloader.h"

VirtAssetSlot VirtAssetSlots::instances[NUM_SLOTS];
FlashVolume VirtAssetSlots::boundVolume;
uint8_t VirtAssetSlots::numBoundSlots;
_SYSCubeIDVector VirtAssetSlots::cubeBanks;


void VirtAssetSlots::bind(FlashVolume volume, unsigned numSlots)
{
    ASSERT(volume.isValid());
    ASSERT(numSlots <= NUM_SLOTS);

    boundVolume = volume;
    numBoundSlots = numSlots;

    rebind(CubeSlots::sysConnected);
}

void VirtAssetSlots::rebind(_SYSCubeIDVector cv)
{
    /*
     * Iterate first over cubes, then over slots. This avoids thrashing
     * our cache, since SysLFS stores allocation info per-cube not per-slot.
     */
    while (cv) {
        _SYSCubeID cube = Intrinsic::CLZ(cv);
        cv ^= Intrinsic::LZ(cube);
        rebindCube(cube);
    }
}

void VirtAssetSlots::rebindCube(_SYSCubeID cube)
{
    ASSERT(cube < _SYS_NUM_CUBE_SLOTS);

    FlashVolume volume = boundVolume;
    unsigned numSlots = numBoundSlots;

    /*
     * Bind() operations always read SysLFS and may write to it.
     * We only want to interact with SysLFS if we're operating on
     * both a nonzero number of cubes and a nonzero number of bound slots.
     */

    if (numSlots) {
        SysLFS::CubeRecord cr;
        SysLFS::Key ck = cr.makeKey(cube);
        unsigned bank = 0;

        bool needWrite = false;
        bool needErase = false;

        if (!SysLFS::read(ck, cr))
            cr.init();

        if (!cr.assets.checkBinding(volume, numSlots)) {
            // Creating a new binding, and erase it.

            cr.assets.allocBinding(volume, numSlots);
            cr.assets.markAccessed(volume, numSlots, true);
            needErase = true;
            needWrite = true;

        } else if (cr.assets.markAccessed(volume, numSlots, false)) {
            // Bump up the access rank of an existing binding

            needWrite = true;
        }

        // Bind all slots
        for (unsigned i = 0; i < arraysize(cr.assets.slots); ++i) {
            SysLFS::AssetSlotOverviewRecord &slot = cr.assets.slots[i];
            if (slot.identity.inActiveSet(volume, numSlots)) {
                ASSERT(slot.identity.ordinal < NUM_SLOTS);
                instances[slot.identity.ordinal].bind(cube, i);
                bank = i / SysLFS::ASSET_SLOTS_PER_BANK;
            }
        }

        if (needErase) {
            // Mark physical slots as erased in CubeRecord, and keep track of them
            PhysSlotVector erasedSlots;
            erasedSlots.clear();
            for (unsigned vslot = 0; vslot < numSlots; vslot++) {
                PhysAssetSlot phys = getInstance(vslot).getPhys(cube);
                ASSERT(phys.isValid());
                erasedSlots.mark(phys.index());
                cr.assets.markErased(phys.index());
            }
            ASSERT(!erasedSlots.empty());

            // Delete AssetSlotRecords for the erased cubes, if necessary
            eraseAssetSlotRecords(cube, erasedSlots);
        }

        if (needWrite) {
            // Write back updated CubeRecord
            SysLFS::write(ck, cr);
        }

        // Update the graphics engine's current cube bank
        setCubeBank(cube, bank);
    }

    // Unbind any remaining slots.
    for (unsigned i = numSlots; i < NUM_SLOTS; ++i)
        instances[i].unbind(cube);
}

bool VirtAssetSlots::physSlotIsBound(_SYSCubeID cube, unsigned physSlot)
{
    unsigned numSlots = numBoundSlots;

    for (unsigned i = 0; i != numSlots; ++i) {
        PhysAssetSlot pSlot = getInstance(i).getPhys(cube);
        if (pSlot.isValid() && pSlot.index() == physSlot)
            return true;
    }

    return false;
}

bool VirtAssetSlots::locateGroup(const AssetGroupInfo &group,
                                 _SYSCubeIDVector searchCV,
                                 _SYSCubeIDVector &foundCV,
                                 const VirtAssetSlot *allocSlot)
{
    /*
     * Iterate through SysLFS until we find the indicated group
     * on all cubes in the vector. If allocSlot is non-NULL, we'll
     * be allocating space for any groups we don't find.
     */

    foundCV = 0;

    FlashLFS &lfs = SysLFS::get();

    // Iterate until we find the group on all cubes or we run out of keys to search.
    FlashLFSIndexRecord::KeyVector_t visited;
    visited.clear();

    while (searchCV) {

        // Restartable iteration, in case we need to collect garbage
        FlashLFSObjectIter iter(lfs);
        while (searchCV) {
            _SYSCubeID cube;
            unsigned slot;
            SysLFS::AssetSlotRecord asr;
            SysLFS::Key asrKey;

            if (iter.previous(FlashLFSKeyQuery())) {
                // Found an existing record

                // Already seen a newer version of this key?
                asrKey = (SysLFS::Key) iter.record()->getKey();
                if (visited.test(asrKey))
                    continue;
                visited.mark(asrKey);

                // Is this an AssetSlotRecord?
                SysLFS::Key cubeKey;
                if (!SysLFS::AssetSlotRecord::decodeKey(asrKey, cubeKey, slot))
                    continue;
                ASSERT(slot < SysLFS::ASSET_SLOTS_PER_CUBE);

                // Find the associated Cube ID
                if (!SysLFS::CubeRecord::decodeKey(cubeKey, cube))
                    continue;
                ASSERT(cube < _SYS_NUM_CUBE_SLOTS);

                // Is this a cube we're still interested in?
                if (0 == (searchCV & Intrinsic::LZ(cube)))
                    continue;

                if (allocSlot) {
                    // Is this the slot we're interested in?
                    PhysAssetSlot pSlot = allocSlot->getPhys(cube);
                    if (!pSlot.isValid() || slot != pSlot.index())
                        continue;

                } else {
                    // Alternately.. is this any bound slot?
                    if (!physSlotIsBound(cube, slot))
                        continue;
                }

                // Yes, still interested! Read in the AssetSlotRecord.
                if (!asr.load(iter))
                    continue;

            } else if (allocSlot) {
                // We're out of records. Create a new record for one cube.

                cube = Intrinsic::CLZ(searchCV);
                SysLFS::Key cubeKey = SysLFS::CubeRecord::makeKey(cube);
                slot = allocSlot->getPhys(cube).index();

                asr.init();
                asrKey = asr.makeKey(cubeKey, slot);

            } else {
                // Out of records, and not allowed to allocate. Done searching.
                return true;
            }

            // Map the cube-specific data for this group and this cube.
            _SYSAssetGroupCube *agc = AssetUtil::mapGroupCube(group.va, cube);
            if (!agc) {
                // Bad pointer from userspace! Give up completely.
                return false;
            }

            // Is this group present already?
            unsigned offset;
            if (asr.findGroup(group.identity(), offset)) {
                agc->baseAddr = offset + slot * PhysAssetSlot::SLOT_SIZE;
                foundCV |= Intrinsic::LZ(cube);
                searchCV ^= Intrinsic::LZ(cube);
                continue;
            }

            if (allocSlot) {
                // If we have a specific slot in mind, we can finish searching on one cube now.
                // Otherwise, we may still need to search other slots on the same cube.
                ASSERT(searchCV & Intrinsic::LZ(cube));
                searchCV ^= Intrinsic::LZ(cube);

                // Try to allocate the group
                if (!asr.allocGroup(group.identity(), group.numTiles, offset)) {
                    // We know for sure that there isn't any room left. Abort!
                    return false;
                }
                agc->baseAddr = offset + slot * PhysAssetSlot::SLOT_SIZE;

                // Mark this slot as a work in progress, remember to finalize later
                asr.flags |= asr.F_LOAD_IN_PROGRESS;

                // Now we need to write back the modified record. (Without GC)
                int size = asr.writeableSize();
                if (SysLFS::write(asrKey, (uint8_t*)&asr, size, false) == size)
                    continue;

                // We failed to write without garbage collection. We may be
                // able to write with GC enabled, but at this point we'd need
                // to restart iteration, in case volumes were deleted.
                SysLFS::write(asrKey, (uint8_t*)&asr, size, true);
                break;
            }
        }
    }

    ASSERT(!searchCV);
    return true;
}

void VirtAssetSlots::finalizeSlot(_SYSCubeID cube, const VirtAssetSlot &slot,
    const AssetGroupInfo &group)
{
    /*
     * Remove the F_LOAD_IN_PROGRESS flag from the indicated slot,
     * and XOR the indicated group's CRC into the slot CRC.
     *
     * This is almost a SysLFS operation we could do using only the
     * basic read/write API, but an AssetSlotRecord is variable-sized,
     * making this a tiny bit more complicated.
     */

    SysLFS::AssetSlotRecord asr;
    SysLFS::Key cubeKey = SysLFS::CubeRecord::makeKey(cube);
    SysLFS::Key asrKey = asr.makeKey(cubeKey, slot.getPhys(cube).index());
    FlashLFS &lfs = SysLFS::get();

    FlashLFSObjectIter iter(lfs);

    while (iter.previous(FlashLFSKeyQuery(asrKey))) {
        if (!asr.load(iter))
            continue;

        // Clear the load-in-progress flag
        if ((asr.flags & asr.F_LOAD_IN_PROGRESS) == 0) {
            LOG(("SYSLFS: Finalizing group that isn't in-progress!\n"));
            continue;
        }
        asr.flags ^= asr.F_LOAD_IN_PROGRESS;

        // Calculate this group's CRC
        uint8_t crc[_SYS_ASSET_GROUP_CRC_SIZE];
        group.copyCRC(crc);

        // XOR it with the slot's buffer
        for (unsigned i = 0; i < arraysize(crc); ++i)
            asr.crc[i] ^= crc[i];

        // Write back, with GC if we need it.
        SysLFS::write(asrKey, (uint8_t*)&asr, asr.writeableSize(), true);
        return;
    }
}

_SYSCubeIDVector VirtAssetSlot::validCubeVector()
{
    /*
     * On which cubes does this Virtual asset slot have a valid physical
     * slot binding?
     */

    _SYSCubeIDVector cv = 0;
    for (unsigned i = 0; i < _SYS_NUM_CUBE_SLOTS; ++i)
        if (phys[i].isValid())
            cv |= Intrinsic::LZ(i);
    return cv;
}

uint32_t VirtAssetSlot::tilesFree(_SYSCubeIDVector cv)
{
    /*
     * Iterate through SysLFS, looking for records that apply to this
     * asset slot. We should see one for every cube the slot has been
     * bound on. Our result is the minimum amount of free space from
     * any of these records which are present in 'cv.
     */

    unsigned minTilesFree = SysLFS::TILES_PER_ASSET_SLOT;
    FlashLFS &lfs = SysLFS::get();
    FlashLFSObjectIter iter(lfs);
    cv &= validCubeVector();

    while (cv && minTilesFree && iter.previous(FlashLFSKeyQuery())) {
        _SYSCubeID cube;
        unsigned slot;

        // Is this an AssetSlotRecord?
        SysLFS::Key asrKey = (SysLFS::Key) iter.record()->getKey();
        SysLFS::Key cubeKey;
        if (!SysLFS::AssetSlotRecord::decodeKey(asrKey, cubeKey, slot))
            continue;

        // Find the associated Cube ID
        if (!SysLFS::CubeRecord::decodeKey(cubeKey, cube))
            continue;
        ASSERT(cube < _SYS_NUM_CUBE_SLOTS);

        // Is this a cube we're still interested in?
        if (0 == (cv & Intrinsic::LZ(cube)))
            continue;

        // Is this the PhysAssetSlot we're mapped to?
        PhysAssetSlot phys = getPhys(cube);
        if (!phys.isValid() || phys.index() != slot)
            continue;

        // Yes, still interested! Read in the AssetSlotRecord.
        SysLFS::AssetSlotRecord asr;
        if (!asr.load(iter))
            continue;

        unsigned tilesFreeOnCube = asr.tilesFree();
        cv ^= Intrinsic::LZ(cube);
        minTilesFree = MIN(minTilesFree, tilesFreeOnCube);
    }

    return minTilesFree;
}

void PhysAssetSlot::getRecordForCube(_SYSCubeID cube, SysLFS::AssetSlotRecord &asr)
{
    /*
     * Read in a single AssetSlotRecord, for this slot on the specified cube.
     */

    FlashLFS &lfs = SysLFS::get();
    FlashLFSObjectIter iter(lfs);

    SysLFS::Key cubeKey = SysLFS::CubeRecord::makeKey(cube);
    SysLFS::Key asrKey = asr.makeKey(cubeKey, index());

    while (iter.previous(FlashLFSKeyQuery(asrKey))) {
        if (asr.load(iter))
            return;
    }

    asr.init();
}

void VirtAssetSlot::getRecordForCube(_SYSCubeID cube, SysLFS::AssetSlotRecord &asr)
{
    return getPhys(cube).getRecordForCube(cube, asr);
}

void VirtAssetSlot::erase(_SYSCubeIDVector cv)
{
    /*
     * Empty out this AssetSlot, on the indicated cubes.
     *
     * This marks the slot as erased in our CubeAssetsRecord, and
     * deletes any AssetSlotRecords for this slot.
     *
     * Iterates over SysLFS only once, doing this all in one step, if possible.
     */

    _SYSCubeIDVector pendingOverview = cv & validCubeVector();
    _SYSCubeIDVector pendingSlots = pendingOverview;

    FlashLFS &lfs = SysLFS::get();
    FlashLFSObjectIter iter(lfs);

    while (pendingOverview | pendingSlots) {

        // Restartable iteration, in case we need to collect garbage
        FlashLFSObjectIter iter(lfs);
        while (pendingOverview | pendingSlots) {
            
            if (!iter.previous(FlashLFSKeyQuery())) {
                // Out of records; done
                return;
            }
            
            _SYSCubeID cube;
            SysLFS::Key key = (SysLFS::Key) iter.record()->getKey();

            // Is this an overview record?
            if (SysLFS::CubeRecord::decodeKey(key, cube)) {
                _SYSCubeIDVector bit = Intrinsic::LZ(cube);

                // Still interested in this overview record?
                if ((bit & pendingOverview) == 0)
                    continue;

                // Yes, still interested! Read in the CubeRecord.
                SysLFS::CubeRecord cr;
                if (!cr.load(iter))
                    continue;

                // No longer pending
                pendingOverview ^= bit;

                PhysAssetSlot phys = getPhys(cube);
                if (phys.isValid()) {
                    // Mark it as erased
                    cr.assets.markErased(phys.index());
                }

                // Now we need to write back the modified record. (Without GC)
                if (SysLFS::write(key, cr, false))
                    continue;

                // We failed to write without garbage collection. We may be
                // able to write with GC enabled, but at this point we'd need
                // to restart iteration, in case volumes were deleted.
                SysLFS::write(key, cr);
                break;
            }

            // Is this an AssetSlotRecord?
            SysLFS::Key cubeKey;
            unsigned slot;
            if (SysLFS::AssetSlotRecord::decodeKey(key, cubeKey, slot)) {

                // Find the associated Cube ID
                if (!SysLFS::CubeRecord::decodeKey(cubeKey, cube))
                    continue;
                ASSERT(cube < _SYS_NUM_CUBE_SLOTS);
                _SYSCubeIDVector bit = Intrinsic::LZ(cube);

                // Is this a cube we're still interested in?
                if ((bit & pendingSlots) == 0)
                    continue;

                // Is this the PhysAssetSlot we're mapped to?
                PhysAssetSlot phys = getPhys(cube);
                if (!phys.isValid() || phys.index() != slot)
                    continue;

                // Yes, still interested! Read in the AssetSlotRecord.
                SysLFS::AssetSlotRecord asr;
                if (!asr.load(iter))
                    continue;

                // No longer pending
                pendingSlots ^= bit;

                // If this slot is already empty, we're done. (Don't write to flash)
                if (asr.isEmpty())
                    continue;

                // Delete this record
                if (SysLFS::write(key, 0, 0, false) == 0)
                    continue;

                // Try again with garbage collection and iterator restart
                SysLFS::write(key, 0, 0, true);
                break;
            }
        }
    }
}

void VirtAssetSlots::eraseAssetSlotRecords(_SYSCubeID cube, PhysSlotVector slots)
{
    /*
     * In one pass through SysLFS, delete all AssetSlotRecords which match the
     * specified cube and any of the specified physical slots.
     */

    FlashLFS &lfs = SysLFS::get();
    FlashLFSObjectIter iter(lfs);

    while (!slots.empty()) {

        // Restartable iteration, in case we need to collect garbage
        FlashLFSObjectIter iter(lfs);
        while (!slots.empty()) {

            if (!iter.previous(FlashLFSKeyQuery())) {
                // Out of records; done
                return;
            }

            SysLFS::Key key = (SysLFS::Key) iter.record()->getKey();
            SysLFS::Key cubeKey;
            unsigned slot;

            // Is this an AssetSlotRecord?
            if (SysLFS::AssetSlotRecord::decodeKey(key, cubeKey, slot)) {
                _SYSCubeID recordCube;

                // Find the associated Cube ID
                if (!SysLFS::CubeRecord::decodeKey(cubeKey, recordCube))
                    continue;

                // Is this a cube we're interested in?
                if (recordCube != cube)
                    continue;

                // Is this a slot we're still interested in?
                if (!slots.test(slot))
                    continue;

                // No longer pending
                slots.clear(slot);

                // Yes, still interested! Read in the AssetSlotRecord.
                SysLFS::AssetSlotRecord asr;
                if (!asr.load(iter))
                    continue;

                // If this slot is already empty, we're done. (Don't write to flash)
                if (asr.isEmpty())
                    continue;

                // Delete this record
                if (SysLFS::write(key, 0, 0, false) == 0)
                    continue;

                // Try again with garbage collection and iterator restart
                SysLFS::write(key, 0, 0, true);
                break;
            }
        }
    }
}

void VirtAssetSlots::setCubeBank(_SYSCubeID cube, unsigned bank)
{
    // Assumes we have only two banks
    STATIC_ASSERT(SysLFS::ASSET_SLOTS_PER_BANK * 2 == SysLFS::ASSET_SLOTS_PER_CUBE);
    ASSERT(bank < 2);
    ASSERT(cube < _SYS_NUM_CUBE_SLOTS);

    if (bank)
        cubeBanks |= Intrinsic::LZ(cube);
    else
        cubeBanks &= ~Intrinsic::LZ(cube);
}

unsigned VirtAssetSlots::getCubeBank(_SYSCubeID cube)
{
    // Assumes we have only two banks
    STATIC_ASSERT(SysLFS::ASSET_SLOTS_PER_BANK * 2 == SysLFS::ASSET_SLOTS_PER_CUBE);
    ASSERT(cube < _SYS_NUM_CUBE_SLOTS);

    return (cubeBanks & Intrinsic::LZ(cube)) != 0;
}
