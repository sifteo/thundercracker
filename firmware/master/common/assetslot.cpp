/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "assetslot.h"
#include "cube.h"
#include "cubeslots.h"
#include "machine.h"
#include "svmruntime.h"
#include "svmloader.h"

VirtAssetSlot VirtAssetSlots::instances[NUM_SLOTS];
FlashVolume VirtAssetSlots::boundVolume;
uint8_t VirtAssetSlots::numBoundSlots;


bool MappedAssetGroup::init(_SYSAssetGroup *userPtr)
{
    if (!isAligned(userPtr)) {
        SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
        return false;
    }
    if (!SvmMemory::mapRAM(userPtr, sizeof *group)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return false;
    }
    
    SvmMemory::VirtAddr hdrVA = userPtr->pHdr;
    group = userPtr;

    if (!SvmMemory::copyROData(header, hdrVA)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return false;
    }

    // Determine the systemwide unique ID for this asset group
    id.ordinal = header.ordinal;
    id.volume = SvmLoader::volumeForVA(hdrVA).block.code;

    if (!id.volume) {
        SvmRuntime::fault(F_SYSCALL_PARAM);
        return false;
    }

    return true;
}

void VirtAssetSlots::bind(FlashVolume volume, unsigned numSlots)
{
    ASSERT(volume.isValid());
    ASSERT(numSlots <= NUM_SLOTS);

    boundVolume = volume;
    numBoundSlots = numSlots;

    // XXX: Should rebind only connected cubes (HWID valid, paired in SysLFS)
    rebind(CubeSlots::vecEnabled);
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

        if (!SysLFS::read(ck, cr))
            cr.init();

        if (!cr.assets.checkBinding(volume, numSlots)) {
            // Creating a new binding
            cr.assets.allocBinding(volume, numSlots);
            cr.assets.markAccessed(volume, numSlots);
            SysLFS::write(ck, cr);
        } else if (cr.assets.markAccessed(volume, numSlots)) {
            // Bump up the access rank of an existing binding
            SysLFS::write(ck, cr);
        }

        for (unsigned i = 0; i < arraysize(cr.assets.slots); ++i) {
            SysLFS::AssetSlotOverviewRecord &slot = cr.assets.slots[i];
            if (slot.identity.inActiveSet(volume, numSlots)) {
                ASSERT(slot.identity.ordinal < NUM_SLOTS);
                instances[slot.identity.ordinal].bind(cube, i);
            }
        }
    }

    // Unbind any remaining slots.
    for (unsigned i = numSlots; i < NUM_SLOTS; ++i)
        instances[i].unbind(cube);
}

bool VirtAssetSlots::locateGroup(MappedAssetGroup &map,
    _SYSCubeIDVector searchCV, _SYSCubeIDVector &foundCV,
    const VirtAssetSlot *vSlot,
    FlashLFSIndexRecord::KeyVector_t *allocVec)
{
    /*
     * Iterate through SysLFS until we find the indicated group
     * on all cubes in the vector. If allocVec is non-NULL, we'll
     * be allocating space for any groups we don't find.
     */

    ASSERT((vSlot == 0) == (allocVec == 0));

    foundCV = 0;

    FlashLFS &lfs = SysLFS::get();

    // Iterate until we find the group on all cubes
    while (searchCV) {

        // Restartable iteration, in case we need to collect garbage
        FlashLFSObjectIter iter(lfs);
        while (searchCV) {
            _SYSCubeID cube;
            unsigned slot;
            SysLFS::AssetSlotRecord asr;
            SysLFS::Key asrKey;

            if (iter.previous()) {
                // Found an existing record

                // Is this an AssetSlotRecord?
                asrKey = (SysLFS::Key) iter.record()->getKey();
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

                // Yes, still interested! Read in the AssetSlotRecord.
                if (!asr.load(iter))
                    continue;

            } else if (allocVec) {
                // We're out of records. Create a new record for one cube.

                cube = Intrinsic::CLZ(searchCV);
                SysLFS::Key cubeKey = SysLFS::CubeRecord::makeKey(cube);
                slot = vSlot->getPhys(cube).index();

                asr.init();
                asrKey = asr.makeKey(cubeKey, slot);

            } else {
                // Out of records, and not allowed to allocate. Done searching.
                return true;
            }

            // Map the cube-specific data for this group and this cube.
            _SYSAssetGroupCube *agc = CubeSlots::instances[cube].assetGroupCube(map.group);
            if (!agc) {
                // Bad pointer from userspace! Give up completely.
                return false;
            }

            // Done searching on this cube
            searchCV ^= Intrinsic::LZ(cube);

            // Is this group present already?
            unsigned offset;
            if (asr.findGroup(map.id, offset)) {
                agc->baseAddr = offset + slot * PhysAssetSlot::SLOT_SIZE;
                foundCV |= Intrinsic::LZ(cube);
                continue;
            }

            if (allocVec) {
                // Try to allocate the group
                if (!asr.allocGroup(map.id, map.header.numTiles, offset)) {
                    // We know for sure that there isn't any room left. Abort!
                    return false;
                }
                agc->baseAddr = offset + slot * PhysAssetSlot::SLOT_SIZE;
 
                // Mark this slot as a work in progress, remember to finalize later
                asr.flags |= asr.F_LOAD_IN_PROGRESS;
                allocVec->mark(asrKey);

                // Now we need to write back the modified record. (Without GC)
                if (SysLFS::write(asrKey, asr, false))
                    continue;

                // We failed to write without garbage collection. We may be
                // able to write with GC enabled, but at this point we'd need
                // to restart iteration, in case volumes were deleted.
                SysLFS::write(asrKey, asr);
                break;
            }
        }
    }

    ASSERT(!searchCV);
    return true;
}

void VirtAssetSlots::finalizeGroup(FlashLFSIndexRecord::KeyVector_t &vec)
{
    /*
     * Iterate through SysLFS, finalizing each group we find in 'vec'.
     */

    FlashLFS &lfs = SysLFS::get();

    while (!vec.empty()) {

        // Restartable iteration, in case we need to collect garbage
        FlashLFSObjectIter iter(lfs);
        while (!vec.empty()) {

            if (!iter.previous()) {
                LOG(("SYSLFS: Missing asset slot record, skipping finalization!\n"));
                vec.clear();
                break;
            }

            // Is this a key we're interested in?
            SysLFS::Key k = (SysLFS::Key) iter.record()->getKey();
            if (!vec.test(k))
                continue;

            // Yes, still interested! Read in the AssetSlotRecord.
            SysLFS::AssetSlotRecord asr;
            if (!asr.load(iter))
                continue;

            // All exit paths below want this key cleared from 'vec'.
            vec.clear(k);

            // Clear the load-in-progress flag
            if ((asr.flags & asr.F_LOAD_IN_PROGRESS) == 0) {
                LOG(("SYSLFS: Finalizing group that isn't in-progress!\n"));
                continue;
            }
            asr.flags ^= asr.F_LOAD_IN_PROGRESS;

            // Now we need to write back the modified record. (Without GC)
            if (SysLFS::write(k, asr, false))
                continue;

            // We failed to write without garbage collection. We may be
            // able to write with GC enabled, but at this point we'd need
            // to restart iteration, in case volumes were deleted.
            SysLFS::write(k, asr);
            break;
        }
    }

    ASSERT(vec.empty());
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

uint32_t VirtAssetSlot::tilesFree()
{
    /*
     * Iterate through SysLFS, looking for records that apply to this
     * asset slot. We should see one for every cube the slot has been
     * bound on. Our result is the minimum amount of free space from
     * any of these records.
     */

    _SYSCubeIDVector cv = validCubeVector();
    unsigned minTilesFree = SysLFS::TILES_PER_ASSET_SLOT;
    FlashLFS &lfs = SysLFS::get();
    FlashLFSObjectIter iter(lfs);

    while (cv && minTilesFree && iter.previous()) {
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

void VirtAssetSlot::erase()
{
    /*
     * Empty out this AssetSlot. This marks the slot as erased in our
     * CubeAssetsRecord, and deletes any AssetSlotRecords for this slot.
     *
     * Iterates over SysLFS only once, doing this all in one step, if possible.
     */

    _SYSCubeIDVector pendingOverview = validCubeVector();
    _SYSCubeIDVector pendingSlots = pendingOverview;

    FlashLFS &lfs = SysLFS::get();
    FlashLFSObjectIter iter(lfs);

    while (pendingOverview | pendingSlots) {

        // Restartable iteration, in case we need to collect garbage
        FlashLFSObjectIter iter(lfs);
        while (pendingOverview | pendingSlots) {
            
            if (!iter.previous()) {
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
                if (SysLFS::write(key, 0, 0, false))
                    continue;

                // Try again with garbage collection and iterator restart
                SysLFS::write(key, 0, 0, true);
                break;
            }
        }
    }
}
