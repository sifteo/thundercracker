/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _ASSETSLOTS_H
#define _ASSETSLOTS_H

#include <sifteo/abi.h>
#include "macros.h"
#include "flash_syslfs.h"


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

    // Bind this to a physical slot, on one cube. Returns 'true' if CubeRecord is modified.
    bool bind(_SYSCubeID cube, SysLFS::CubeRecord &cr, SysLFS::AssetSlotIdentity &id);
    
    // Unbind this slot, on one cube.
    void unbind(_SYSCubeID cube);

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

    // Rebind all slots to a different volume, and change the number of active slots
    static void bind(FlashVolume volume, unsigned numSlots);

    // Refresh the current binding, if any, on some specified set of cubes.
    // Called when new cubes are paired.
    static void rebind(_SYSCubeIDVector cv);

private:
    VirtAssetSlots();  // Do not implement

    static VirtAssetSlot instances[NUM_SLOTS];
    static FlashVolume boundVolume;
    static uint8_t numBoundSlots;
};


#endif
