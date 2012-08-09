/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _ASSETSLOTS_H
#define _ASSETSLOTS_H

#include <sifteo/abi.h>
#include "macros.h"
#include "flash_syslfs.h"
#include "svmmemory.h"

struct AssetGroupInfo;


/**
 * Physical AssetSlots correspond to a range of addresses within the cube's
 * flash memory, matching the size of a logical AssetSlot.
 *
 * We store them as a packed 8-bit value, with zero as an invalid value.
 */

class PhysAssetSlot
{
public:
    static const unsigned NUM_SLOTS = SysLFS::ASSET_SLOTS_PER_CUBE;
    static const unsigned SLOT_SIZE = SysLFS::TILES_PER_ASSET_SLOT;

    unsigned address() const {
        STATIC_ASSERT(NUM_SLOTS <= (1ULL << (sizeof(code) * 8)));
        return index() * SLOT_SIZE;
    }

    unsigned index() const {
        ASSERT(isValid());
        return code - 1;
    }

    bool isValid() const {
        return (unsigned)(code - 1) < NUM_SLOTS;
    }

    void setInvalid() {
        code = 0;
    }

    void setIndex(unsigned i) {
        code = i + 1;
    }

    void getRecordForCube(_SYSCubeID cube, SysLFS::AssetSlotRecord &asr);
    void erase(_SYSCubeID cube);

private:
    uint8_t code;     // invalid=0 , valid=[1, NUM_SLOTS]
};


/**
 * A Virtual AssetSlot is owned by a particular volume, and identified by an
 * ordinal number local to that volume.
 *
 * These per-application AssetSlots are "bound" to concrete slots on each
 * cube by the slot allocator. The application then gets to choose how these
 * slots are filled and when they're erased.
 *
 * All persistent data for the AssetSlots are stored in SysLFS.
 *
 * Note that there are more Physical AssetSlots than Virtual, since an
 * application may only use one memory bank at a time.
 */

class VirtAssetSlot
{
public:
    static const unsigned SLOT_SIZE = SysLFS::TILES_PER_ASSET_SLOT;

    void bind(_SYSCubeID cube, unsigned index) {
        phys[cube].setIndex(index);
    }

    void unbind(_SYSCubeID cube) {
        phys[cube].setInvalid();
    }
    
    PhysAssetSlot getPhys(_SYSCubeID cube) const {
        ASSERT(cube < _SYS_NUM_CUBE_SLOTS);
        return phys[cube];
    }

    _SYSCubeIDVector validCubeVector();
    uint32_t tilesFree(_SYSCubeIDVector cv);
    void erase(_SYSCubeIDVector cv);

    void getRecordForCube(_SYSCubeID cube, SysLFS::AssetSlotRecord &asr);

private:
    // Which physical slot does this map to, on each cube?
    PhysAssetSlot phys[_SYS_NUM_CUBE_SLOTS];
};


/**
 * The global pool of virtual asset slots, for the currently executing
 * program. Normally these are "bound" to the same volume that we're
 * running code from, though this isn't the case when one app is
 * bootstrapping assets on behalf of another.
 *
 * We only need enough total Virtual slots for one bank of cube memory,
 * since a single app cannot use more than one bank at a time on any single
 * cube.
 */

class VirtAssetSlots
{
public:
    static const unsigned NUM_SLOTS = SysLFS::ASSET_SLOTS_PER_BANK;

    static VirtAssetSlot &getInstance(_SYSAssetSlot slot) {
        ASSERT(slot < NUM_SLOTS);
        return instances[slot];
    }

    static bool isSlotBound(_SYSAssetSlot slot) {
        ASSERT(numBoundSlots <= NUM_SLOTS);
        return slot < numBoundSlots;
    }

    static unsigned getNumBoundSlots() {
        ASSERT(numBoundSlots <= NUM_SLOTS);
        return numBoundSlots;
    }

    // Rebind all slots to a different volume, and change the number of active slots
    static void bind(FlashVolume volume, unsigned numSlots);

    // Refresh the current binding, if any, on some specified set of cubes.
    // Called when new cubes are paired.
    static void rebind(_SYSCubeIDVector cv);
    static void rebindCube(_SYSCubeID cube);

    // Which A21 bank is the indicated cube using?
    static unsigned getCubeBank(_SYSCubeID cube);

    /**
     * This is the main workhorse for asset lookup and loading. In a single
     * pass, this scans through SysLFS, operating on any slots which are bound
     * and part of 'cv'.
     *
     * If the group can be found in SysLFS, we update its base address in RAM,
     * and update the access ranks if necessary. We return a vector of cubes
     * where the group was already installed.
     *
     * If it can't be found, but 'allocSlot' is non-null we allocate space
     * for the group in that slot, and assign it an address.
     *
     * Since the state of a cubeSlot is indeterminate during a load operation
     * (we don't know whether the physical flash sectors have been erased or
     * not), we want to ensure that we treat the entire slot's contents as
     * unknown if a power failure or other interruption happens during a write.
     *
     * So, for any cube slots where we have to allocate space, we write a new
     * slot record which has (a) the space allocated, and (b) the
     * F_LOAD_IN_PROGRESS flag set. We clear this flag when the loading has
     * successfully finished, in finalizeSlot(). If we see this flag while
     * searching an asset group, we won't trust the contents of the slot at all.
     *
     * Returns 'true' on success. On allocation failure, returns 'false'.
     * Changes may have been already made by the time we discover the failure.
     */

    static bool locateGroup(const AssetGroupInfo &group,
                            _SYSCubeIDVector searchCV,
                            _SYSCubeIDVector &foundCV,
                            const VirtAssetSlot *allocSlot = 0);

    // If the indicated slot is in-progress, finalize writing 'group' to it.
    static void finalizeSlot(_SYSCubeID cube, const VirtAssetSlot &slot,
        const AssetGroupInfo &group);

private:
    VirtAssetSlots();  // Do not implement

    typedef BitVector<SysLFS::ASSET_SLOTS_PER_CUBE> PhysSlotVector;

    static void setCubeBank(_SYSCubeID cube, unsigned bank);
    static void eraseAssetSlotRecords(_SYSCubeID cube, PhysSlotVector slots);
    static bool physSlotIsBound(_SYSCubeID cube, unsigned physSlot);

    static VirtAssetSlot instances[NUM_SLOTS];
    static FlashVolume boundVolume;
    static uint8_t numBoundSlots;
    static _SYSCubeIDVector cubeBanks;
};


#endif
