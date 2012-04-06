/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for asset group and slot operations.
 *
 * XXX: Stub implementation! No persistent cache, only one slot allowed.
 */

#include <sifteo/abi.h>
#include "svmmemory.h"
#include "cube.h"
#include "cubeslots.h"

extern "C" {


struct FakeAssetSlot {
    static const unsigned numTiles = 16384;
    uint16_t nextAddr;
    uint8_t numLoadedGroups;
    struct {
        uint16_t addr;
        uint8_t ordinal;
    } loadedGroups[8];
};
    
FakeAssetSlot FakeAssetSlots[_SYS_NUM_CUBE_SLOTS];


uint32_t _SYS_asset_slotTilesFree(_SYSAssetSlot slot)
{
    /*
     * Return the minimum number of tiles free in this AssetSlot on all cubes.
     */

    uint32_t result = (uint32_t)-1;

    for (_SYSCubeID cube = 0; cube < _SYS_NUM_CUBE_SLOTS; ++cube) {
        FakeAssetSlot &slot = FakeAssetSlots[cube];

        result = MIN(result, slot.numTiles - slot.nextAddr);
    }

    return result;
}

void _SYS_asset_slotErase(_SYSAssetSlot slot)
{
    /*
     * Erase this AssetSlot on all cubes.
     */

    for (_SYSCubeID cube = 0; cube < _SYS_NUM_CUBE_SLOTS; ++cube) {
        FakeAssetSlot &slot = FakeAssetSlots[cube];

        slot.nextAddr = 0;
        slot.numLoadedGroups = 0;
    }
}

uint32_t _SYS_asset_loadStart(_SYSAssetLoader *loader, _SYSAssetGroup *group,
    _SYSAssetSlot slot, _SYSCubeIDVector cv)
{
    if (!SvmMemory::mapRAM(loader))
        return false;
    if (!SvmMemory::mapRAM(group))
        return false;

    _SYSAssetGroupHeader header;
    if (!SvmMemory::copyROData(header, group->pHdr))
        return 0;

    /*
     * Skip groups that are already loaded.
     *
     * Note that these groups are marked as 'complete' in _SYSAssetLoader
     * without their _SYSAssetLoaderCube ever being touched- so their cubeVec
     * bit is still zero!
     */

    _SYSCubeIDVector cachedCV = _SYS_asset_findInCache(group, cv);
    cv &= ~cachedCV;
    loader->complete |= cachedCV;

    /*
     * Try to allocate space in this slot on every requested cube.
     * If this is going to fail, it needs to do so without making any changes.
     */

    uint16_t baseAddrs[_SYS_NUM_CUBE_SLOTS];

    _SYSCubeIDVector allocCV = cv;
    while (allocCV) {
        _SYSCubeID id = Intrinsic::CLZ(allocCV);
        allocCV ^= Intrinsic::LZ(id);
        FakeAssetSlot &slot = FakeAssetSlots[id];

        if (slot.numTiles - slot.nextAddr < header.numTiles)
            return false;

        baseAddrs[id] = slot.nextAddr;
        slot.nextAddr += header.numTiles;
    }

    /*
     * Begin the asset loading itself,
     * and commit to using the memory we just reserved.
     */

    CubeSlots::assetLoader = loader;
    _SYSCubeIDVector loadCV = cv;
    while (loadCV) {
        _SYSCubeID id = Intrinsic::CLZ(loadCV);
        loadCV ^= Intrinsic::LZ(id);
        CubeSlot &cube = CubeSlots::instances[id];
        FakeAssetSlot &slot = FakeAssetSlots[id];

        unsigned n = slot.numLoadedGroups;
        uint16_t addr = baseAddrs[id];
        slot.loadedGroups[n].addr = addr;
        slot.loadedGroups[n].ordinal = header.ordinal;
        slot.numLoadedGroups++;

        cube.startAssetLoad(reinterpret_cast<SvmMemory::VirtAddr>(group), addr);
    }

    return true;
}

void _SYS_asset_loadFinish(_SYSAssetLoader *loader)
{
    CubeSlots::assetLoader = NULL;
}

uint32_t _SYS_asset_findInCache(_SYSAssetGroup *group, _SYSCubeIDVector cv)
{
    /*
     * Find this asset group in the cache, for all cubes marked in 'cv'.
     * Return a _SYSCubeIDVector of all cubes for whom this asset group
     * is already loaded. For each of these cubes, the _SYSAssetGroupCube's
     * baseAddr is updated.
     */

    if (!SvmMemory::mapRAM(group, sizeof *group))
        return 0;

    _SYSAssetGroupHeader header;
    if (!SvmMemory::copyROData(header, group->pHdr))
        return 0;

    _SYSCubeIDVector result = 0;

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

    return result;
}


}  // extern "C"
