/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "assetslot.h"
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
        SysLFS::Key ck = cr.key(cube);
        cr.read(ck);

        if (!cr.assets.checkBinding(volume, numSlots)) {
            cr.assets.allocBinding(volume, numSlots);
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


#if 0
   while (cv) {
        _SYSCubeID id = Intrinsic::CLZ(cv);
        cv ^= Intrinsic::LZ(id);
        FakeAssetSlot &slot = FakeAssetSlots[id];

        for (unsigned I = 0, E = slot.numLoadedGroups; I != E; ++I)
            if (slot.loadedGroups[I].ordinal == header.ordinal) {
                _SYSAssetGroupCube *gc = CubeSlots::instances[id].assetGroupCube(group);
                if (gc)
                    gc->baseAddr = slot.loadedGroups[I].addr;

                result |= Intrinsic::LZ(id);
                break;
            }
    }
#endif