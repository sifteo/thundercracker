/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
using namespace Sifteo;


void logVolumes(_SYSVolumeHandle *volumes, unsigned numVolumes)
{
    for (unsigned i = 0; i < numVolumes; ++i) {
        _SYSVolumeHandle vol = volumes[i];
        static _SYSUUID noUUID;

        uint32_t o = _SYS_elf_map(vol);
        const char *title = (const char*) _SYS_elf_metadata(vol, _SYS_METADATA_TITLE_STR, 1, NULL);
        const _SYSUUID *uuid = (const _SYSUUID*) _SYS_elf_metadata(vol, _SYS_METADATA_UUID, sizeof *uuid, NULL);
        if (!title) title = "(untitled)";
        if (!uuid) uuid = &noUUID;

        LOG("LAUNCHER: Found game vol=%08x, o=%08x, {%4h-%2h-%2h-%2h-% 6h} \"%s\"\n",
            vol, o,
            uuid->bytes + 0,
            uuid->bytes + 4,
            uuid->bytes + 6,
            uuid->bytes + 8,
            uuid->bytes + 10,
            title);
    }
}

_SYSCubeIDVector getCubeVector(_SYSVolumeHandle vol)
{
    /*
     * XXX: BIG HACK.
     *
     * Temporary code to initialize cubes, using the CubeRange from
     * a game's metadata.
     */

    const _SYSMetadataCubeRange *range = (const _SYSMetadataCubeRange *)
        _SYS_elf_metadata(vol, _SYS_METADATA_CUBE_RANGE, sizeof *range, NULL);
    unsigned minCubes = range ? range->minCubes : 0;

    if (!minCubes) {
        LOG("LAUNCHER: No CubeRange found, not initializing any cubes.\n");
        return 0;
    }

    _SYSCubeIDVector cubes = 0xFFFFFFFF << (32 - minCubes);
    return cubes;
}

void bootstrapAssetGroup(const _SYSMetadataBootAsset &bootAsset,
    _SYSCubeIDVector cubes, uint32_t mapOffset)
{
    // Construct an AssetGroup on the stack
    AssetGroup group;
    bzero(group);
    group.sys.pHdr = bootAsset.pHdr + mapOffset;
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

void bootstrapAssets(_SYSVolumeHandle vol, _SYSCubeIDVector cubes, uint32_t mapOffset)
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
    uint32_t actualSize;
    const _SYSMetadataBootAsset *vec = (const _SYSMetadataBootAsset *)
        _SYS_elf_metadata(vol, _SYS_METADATA_BOOT_ASSET, sizeof *vec, &actualSize);
    if (!vec) {
        LOG(("LAUNCHER: No bootstrap assets found\n"));
        return;
    }
    unsigned count = actualSize / sizeof *vec;

    if (!cubes) {
        LOG(("LAUNCHER: Not loading bootstrap assets, no CubeRange found\n"));
        return;
    }

    SCRIPT(LUA, System():setAssetLoaderBypass(true));

    for (unsigned i = 0; i < count; i++)
        bootstrapAssetGroup(vec[i], cubes, mapOffset);

    SCRIPT(LUA, System():setAssetLoaderBypass(false));
}

void exec(_SYSVolumeHandle vol)
{
    uint32_t mapOffset = _SYS_elf_map(vol);

    LOG("LAUNCHER: Running volume %08x\n", vol);

    // Enable the game's minimum set of cubes
    _SYSCubeIDVector cv = getCubeVector(vol);
    _SYS_enableCubes(cv);

    // Temporary asset bootstrapper
    bootstrapAssets(vol, cv, mapOffset);

    _SYS_elf_exec(vol);
}

void main()
{
    _SYSVolumeHandle volumes[64];
    unsigned numVolumes = _SYS_fs_listVolumes(_SYS_FS_VOL_GAME, volumes, arraysize(volumes));

    if (numVolumes) {
        logVolumes(volumes, numVolumes);
        exec(volumes[0]);

    } else {
        LOG("LAUNCHER: No games installed\n");
        while (1)
            System::paint();
    }
}
