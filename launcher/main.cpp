/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
using namespace Sifteo;


_SYSCubeIDVector getCubeVector(MappedVolume &map)
{
    /*
     * XXX: BIG HACK.
     *
     * Temporary code to initialize cubes, using the CubeRange from
     * a game's metadata.
     */

    auto range = map.metadata<_SYSMetadataCubeRange>(_SYS_METADATA_CUBE_RANGE);
    unsigned minCubes = range ? range->minCubes : 0;

    if (!minCubes) {
        LOG("LAUNCHER: No CubeRange found, not initializing any cubes.\n");
        return 0;
    }

    _SYSCubeIDVector cubes = 0xFFFFFFFF << (32 - minCubes);
    return cubes;
}

void bootstrapAssetGroup(MappedVolume &map, _SYSCubeIDVector cubes,
    const _SYSMetadataBootAsset &bootAsset)
{
    // Construct an AssetGroup on the stack
    AssetGroup group;
    bzero(group);
    group.sys.pHdr = map.translate(bootAsset.pHdr);
    AssetSlot slot(bootAsset.slot);

    if (group.isInstalled(cubes)) {
        LOG("LAUNCHER: Bootstrap asset group %P already installed\n", group.sys.pHdr);
        return;
    }

    LOG("LAUNCHER: Installing bootstrap asset group %P in slot %d\n",
        group.sys.pHdr, slot.sys);

    ScopedAssetLoader loader;
    if (!loader.start(group, slot, cubes)) {
        // Out of space. Erase the slot first.
        LOG("LAUNCHER: Erasing asset slot\n");
        slot.erase();
        loader.start(group, slot, cubes);
    }

    while (!loader.isComplete())
        System::paint();
}

void bootstrapAssets(MappedVolume &map, _SYSCubeIDVector cubes)
{
    /*
     * XXX: BIG HACK.
     *
     * Temporary code to load the bootstrap assets specified
     * in a game's metadata.
     *
     * In a real loader scenario, bootstrap assets would be loaded
     * onto a set of cubes determined by the loader: likely calculated
     * by looking at the game's supported cube range and the number of
     * connected cubes.
     *
     * In this hack, we just load the bootstrap assets onto the
     * first N cubes, where N is the minimum number required by the game.
     * If no cube range was specified, we don't load assets at all.
     */

    // Look up BootAsset array
    uint32_t actual;
    auto vec = map.metadata<_SYSMetadataBootAsset>(_SYS_METADATA_BOOT_ASSET, &actual);
    if (!vec) {
        LOG(("LAUNCHER: No bootstrap assets found\n"));
        return;
    }
    unsigned count = actual / sizeof *vec;

    if (!cubes) {
        LOG(("LAUNCHER: Not loading bootstrap assets, no CubeRange found\n"));
        return;
    }

    SCRIPT(LUA, System():setAssetLoaderBypass(true));

    for (unsigned i = 0; i < count; i++)
        bootstrapAssetGroup(map, cubes, vec[i]);

    SCRIPT(LUA, System():setAssetLoaderBypass(false));
}

void exec(Volume vol)
{
    MappedVolume map(vol);

    LOG("LAUNCHER: Running volume %08x\n", vol.sys);

    // Enable the game's minimum set of cubes
    _SYSCubeIDVector cv = getCubeVector(map);
    _SYS_enableCubes(cv);

    // Temporary asset bootstrapper
    bootstrapAssets(map, cv);

    vol.exec();
}

void main()
{
    Array<Volume, 64> gGameList;
    Volume::list(Volume::T_GAME, gGameList);

    if (gGameList.empty()) {
        LOG("LAUNCHER: No games installed\n");
        while (1)
            System::paint();
    }

    for (Volume v : gGameList) {
        MappedVolume map(v);
        LOG("LAUNCHER: Found game vol=%08x, {%16h} \"%s\"\n",
            v.sys, map.uuid()->bytes, map.title());
    }

    exec(gGameList[0]);
}
