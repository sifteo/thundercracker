/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "assetslot.h"
#include "cubeslots.h"
#include "machine.h"

VirtAssetSlot VirtAssetSlots::instances[NUM_SLOTS];
FlashVolume VirtAssetSlots::boundVolume;
uint8_t VirtAssetSlots::numBoundSlots;


bool VirtAssetSlot::bind(_SYSCubeID cube, SysLFS::CubeRecord &cr, SysLFS::AssetSlotIdentity &id)
{
    //LOG(("VirtAssetSlot bind(cube=%d, id=%d:%d)\n", cube, id.volume, id.ordinal));

    return false;
}

void VirtAssetSlot::unbind(_SYSCubeID cube)
{
    ASSERT(cube < arraysize(phys));
    phys[cube].setInvalid();
}

void VirtAssetSlots::bind(FlashVolume volume, unsigned numSlots)
{
    ASSERT(volume.isValid());
    ASSERT(numSlots <= NUM_SLOTS);

    boundVolume = volume;
    numBoundSlots = numSlots;

    // XXX: Should rebind only paired cubes (HWID valid, paired in SysLFS)
    rebind(CubeSlots::vecEnabled);
}

void VirtAssetSlots::rebind(_SYSCubeIDVector cv)
{
    /*
     * Iterate first over cubes, then over slots. This avoids thrashing
     * our cache, since SysLFS stores allocation info per-cube not per-slot.
     */

    SysLFS::AssetSlotIdentity id;
    id.volume = boundVolume.block.code;

    while (cv) {
        unsigned cube = Intrinsic::CLZ(cv);
        cv ^= Intrinsic::LZ(cube);
        id.ordinal = 0;

        /*
         * Bind() operations always read SysLFS and may write to it.
         * We only want to interact with SysLFS if we're operating on
         * both a nonzero number of cubes and a nonzero number of bound slots.
         */
        if (numBoundSlots) {
            SysLFS::CubeRecord cr;
            bool modified = false;

            cr.read(cube);

            for (id.ordinal = 0; id.ordinal < numBoundSlots; ++id.ordinal)
                modified |= instances[id.ordinal].bind(cube, cr, id);

            if (modified)
                cr.write(cube);
        }

        // Unbind any remaining slots.
        for (; id.ordinal < NUM_SLOTS; ++id.ordinal)
            instances[id.ordinal].unbind(cube);
    }
}
