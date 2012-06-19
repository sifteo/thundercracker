/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for asset group and slot operations.
 */

#include <sifteo/abi.h>
#include "svmmemory.h"
#include "cube.h"
#include "cubeslots.h"
#include "svmruntime.h"
#include "flash_syslfs.h"
#include "assetslot.h"
#include "tasks.h"

static FlashLFSIndexRecord::KeyVector_t gSlotsInProgress;


extern "C" {


void _SYS_asset_bindSlots(_SYSVolumeHandle volHandle, unsigned numSlots)
{
    FlashVolume vol(volHandle);
    if (!vol.isValid())
        return SvmRuntime::fault(F_BAD_VOLUME_HANDLE);

    if (numSlots > VirtAssetSlots::NUM_SLOTS)
        return SvmRuntime::fault(F_SYSCALL_PARAM);

    VirtAssetSlots::bind(vol, numSlots);
}

uint32_t _SYS_asset_slotTilesFree(_SYSAssetSlot slot)
{
    if (!VirtAssetSlots::isSlotBound(slot)) {
        SvmRuntime::fault(F_BAD_ASSETSLOT);
        return 0;
    }

    return VirtAssetSlots::getInstance(slot).tilesFree();
}

void _SYS_asset_slotErase(_SYSAssetSlot slot)
{
    if (!VirtAssetSlots::isSlotBound(slot))
        return SvmRuntime::fault(F_BAD_ASSETSLOT);

    VirtAssetSlots::getInstance(slot).erase();
}

uint32_t _SYS_asset_loadStart(_SYSAssetLoader *loader, _SYSAssetGroup *group,
    _SYSAssetSlot slot, _SYSCubeIDVector cv)
{
    if (!isAligned(loader)) {
        SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
        return false;
    }
    if (!SvmMemory::mapRAM(loader)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return false;
    }

    /*
     * Finish first, if a different load is in progress.
     *
     * We must be able to support separate 'start' calls on the same loader
     * for different cubes. (Not all callers will know to combine all cubes
     * into a single CubeIDVector)
     */
    _SYSAssetLoader *prevLoader = CubeSlots::assetLoader;
    if (prevLoader && prevLoader != loader) {
        _SYS_asset_loadFinish(prevLoader);
        ASSERT(CubeSlots::assetLoader == 0);
        ASSERT(gSlotsInProgress.empty());
    }

    MappedAssetGroup map;
    if (!map.init(group))
        return false;

    if (!VirtAssetSlots::isSlotBound(slot)) {
        SvmRuntime::fault(F_BAD_ASSETSLOT);
        return false;
    }
    const VirtAssetSlot &vSlot = VirtAssetSlots::getInstance(slot);

    cv = CubeSlots::truncateVector(cv);

    /*
     * In one step, scan the SysLFS to the indicated group.
     *
     * If the group is cached, it's written to 'cachedCV'. If not,
     * space is allocated for it. In either case, this updates the
     * address of this group on each of the indicated cubes.
     *
     * For any cubes where this group needs to be loaded, we'll mark
     * the relevant AssetSlots as 'in progress'. A set of these
     * in-progress keys are written to gSlotsInProgress, so tha
     * we can finalize them after the loading has finished.
     *
     * If this fails to allocate space, we return unsuccessfully.
     * Affected groups may be left in the indeterminate state.
     */

    _SYSCubeIDVector cachedCV;
    if (!VirtAssetSlots::locateGroup(map, cv, cachedCV, &vSlot, &gSlotsInProgress))
        return false;

    cv &= ~cachedCV;
    loader->complete |= cachedCV;

    /*
     * Begin the asset loading itself
     */

    CubeSlots::assetLoader = loader;

    while (cv) {
        _SYSCubeID id = Intrinsic::CLZ(cv);
        cv ^= Intrinsic::LZ(id);

        CubeSlot &cube = CubeSlots::instances[id];
        _SYSAssetGroupCube *gc = cube.assetGroupCube(map.group);
        if (gc) {
            cube.startAssetLoad(reinterpret_cast<SvmMemory::VirtAddr>(map.group), gc->baseAddr);
        }
    }

    return true;
}

void _SYS_asset_loadFinish(_SYSAssetLoader *loader)
{
    if (!isAligned(loader)) {
        SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
        return;
    }
    if (!SvmMemory::mapRAM(loader)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return;
    }
    
    if (!CubeSlots::assetLoader) {
        // No load in progress. No effect.
        return;
    }

    // Must be the correct loader instance!
    if (CubeSlots::assetLoader != loader) {
        return SvmRuntime::fault(F_SYSCALL_PARAM);
        return;
    }

    /*
     * Block until the load operation is finished, if it isn't already.
     * We can't rely on userspace to do this before we mark the in-progress
     * slots as no-longer in progress.
     */

    while ((loader->complete & loader->cubeVec) != loader->cubeVec)
        Tasks::idle();

    // No more current load operation
    CubeSlots::assetLoader = NULL;

    // Finalize the SysLFS state for any slots we're loading to
    VirtAssetSlots::finalizeGroup(gSlotsInProgress);
    ASSERT(gSlotsInProgress.empty());
}

uint32_t _SYS_asset_findInCache(_SYSAssetGroup *group, _SYSCubeIDVector cv)
{
    /*
     * Find this asset group in the cache, for all cubes marked in 'cv'.
     * Return a _SYSCubeIDVector of all cubes for whom this asset group
     * is already loaded. For each of these cubes, the _SYSAssetGroupCube's
     * baseAddr is updated.
     */

    MappedAssetGroup map;
    if (!map.init(group))
        return 0;

    cv = CubeSlots::truncateVector(cv);

    _SYSCubeIDVector cachedCV;
    if (!VirtAssetSlots::locateGroup(map, cv, cachedCV))
        cachedCV = 0;

    return cachedCV;
}


}  // extern "C"
