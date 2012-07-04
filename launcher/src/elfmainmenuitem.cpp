/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "elfmainmenuitem.h"
#include "mainmenu.h"
#include "assetloaderbypassdelegate.h"
#include <sifteo.h>
#include <sifteo/menu.h>
using namespace Sifteo;

ELFMainMenuItem ELFMainMenuItem::instances[MAX_INSTANCES];


void ELFMainMenuItem::autoexec()
{
    /*
     * This entire function has no effect on physical hardware.
     * In simulation, we check to see if exactly one game volume
     * is installed. If so, we silently run that game.
     */

    if (!System::isSimDebug())
        return;

    Array<Volume, 2> volumes;
    Volume::list(Volume::T_GAME, volumes);
    if (volumes.count() != 1)
        return;

    ELFMainMenuItem &inst = instances[0];
    if (!inst.init(volumes[0]))
        return;

    LOG("LAUNCHER: Automatically executing single game\n");

    AssetLoaderBypassDelegate delegate;
    CubeSet cubes = inst.getCubeRange().initMinimum();
    inst.bootstrap(cubes, delegate);
    inst.exec();
}

void ELFMainMenuItem::findGames(MainMenu &menu)
{
    /*
     * Get the list of games from our filesystem. (Limited
     * to the max number of main menu items we can store)
     */

    Array<Volume, MAX_INSTANCES> volumes;
    Volume::list(Volume::T_GAME, volumes);

    /*
     * Create an ELFMainMenuItem for each, skipping any volumes
     * that cause init() to return false.
     */

    unsigned volI = 0, itemI = 0;
    unsigned volE = volumes.count();

    while (volI != volE) {
        ELFMainMenuItem *inst = &instances[itemI];
        if (inst->init(volumes[volI])) {
            menu.append(inst);
            itemI++;
        }
        volI++;
    }
}

bool ELFMainMenuItem::init(Sifteo::Volume volume)
{
    /*
     * Load critical metadata from this volume into RAM, and check whether
     * it's suitable to include in the launcher menu. This loads only
     * lightweight resources from the volume. No assets are stored yet.
     */

    STATIC_ASSERT(MAX_INSTANCES < MainMenu::MAX_ITEMS);

    this->volume = volume;
    MappedVolume map(volume);

    LOG("LAUNCHER: Found Volume<%02x> %s, version %s \"%s\"\n",
        volume.sys & 0xFF, map.package(), map.version(), map.title());

    // Save the cube range (required)
    cubeRange = map.metadata<_SYSMetadataCubeRange>(_SYS_METADATA_CUBE_RANGE);
    if (!cubeRange.isValid()) {
        LOG("LAUNCHER: Skipping game, invalid cube range\n");
        return false;
    }

    // Save the number of asset slots (required)
    const uint8_t *slots = map.metadata<uint8_t>(_SYS_METADATA_NUM_ASLOTS);
    numAssetSlots = slots ? *slots : 0;
    if (numAssetSlots > MAX_ASSET_SLOTS) {
        LOG("LAUNCHER: Skipping game, too many asset slots. Requested %d, max is %d.\n",
            numAssetSlots, MAX_ASSET_SLOTS);
        return false;
    }

    // Validate the required icon, but don't save it yet.
    auto iconMeta = map.metadata<_SYSMetadataImage>(_SYS_METADATA_ICON_96x96);
    if (!iconMeta) {
        LOG("LAUNCHER: Skipping game, no 96x96 icon found.\n");
        return false;
    }
    if (iconMeta->width != icon.image.tileWidth() || iconMeta->height != icon.image.tileHeight()) {
        LOG("LAUNCHER: Skipping game, icon size is incorrect.\n");
        return false;
    }

    // Calculate how much space the icon's assets require
    AssetImage iconImage;
    AssetGroup iconGroup;
    map.translate(iconMeta, iconImage, iconGroup);
    unsigned tileAllocation = iconGroup.tileAllocation();

    // Check the size of this group. It should be no larger than the worst-case uncompressed size
    unsigned maxTileAllocation = roundup(icon.image.numTiles(), _SYS_ASSET_GROUP_SIZE_UNIT);
    if (tileAllocation > maxTileAllocation) {
        LOG("LAUNCHER: Skipping game, icon AssetGroup is too large. "
            "Make sure your icon is in an AssetGroup by itself! Found a "
            "group with %d tiles, expected no more than %d.",
            tileAllocation, maxTileAllocation);
    }

    return true;
}

MainMenuItem::Flags ELFMainMenuItem::getAssets(Sifteo::MenuItem &assets, Sifteo::MappedVolume &map)
{
    /*
     * Gather assets from this volume.
     */

    map.attach(volume);

    // We already validated the icon metadata
    auto iconMeta = map.metadata<_SYSMetadataImage>(_SYS_METADATA_ICON_96x96);
    ASSERT(iconMeta);
    ASSERT(iconMeta->width == icon.image.tileWidth());
    ASSERT(iconMeta->height == icon.image.tileHeight());

    // Mapping translation; convert to an AssetImage and AssetGroup.
    AssetImage iconSrc;
    map.translate(iconMeta, iconSrc, icon.group);

    // The above AssetImage still references data in the mapped volume,
    // which won't be available later. Copy / decompress it into RAM.
    icon.image.init();
    icon.image.image(vec(0,0), iconSrc);
    assets.icon = icon.image;

    return LOAD_ASSETS;
}

void ELFMainMenuItem::bootstrap(Sifteo::CubeSet cubes, ProgressDelegate &progress)
{
    /*
     * Bootstrap any number of asset groups from this volume.
     */

    MappedVolume map(volume);

    // Look up BootAsset array
    uint32_t actual;
    auto vec = map.metadata<_SYSMetadataBootAsset>(_SYS_METADATA_BOOT_ASSET, &actual);
    if (!vec) {
        LOG(("LAUNCHER: No bootstrap assets found\n"));
        return;
    }
    unsigned count = actual / sizeof *vec;

    if (cubes.empty()) {
        LOG(("LAUNCHER: No cubes to load bootstrap assets on\n"));
        return;
    }

    /*
     * Now bind this volume's asset slots. After this point, we can't access
     * any of the launcher's AssetGroups without reverting to our own binding.
     *
     * XXX: Currently the firmware requires cubes to be enabled before bindSlots.
     *      It needs to support cubes dynamically coming and going.
     *
     * XXX: Need a separate way to control which A21 bank we use for drawing.
     *      That shouldn't change until just before we exec.
     */
    _SYS_enableCubes(cubes);
    _SYS_asset_bindSlots(volume, numAssetSlots);

    /*
     * First pass, size accounting.
     *
     * This calculates a total size of all unloaded asset groups, and keeps track
     * of which groups and which slots are participating in bootstrapping. This
     * gives us enough information to calculate a global progress amount, plus
     * to determine ahead-of-time whether we need to clear any AssetSlots.
     *
     * We have to keep track of separate counts for installed vs. total asset
     * groups, since at this point we don't know which slots, if any, will need
     * to be erased.
     */

    bool needAnyInstall = false;

    BitArray<MAX_BOOTSTRAP_GROUPS> installedGroups;
    installedGroups.clear();

    BitArray<MAX_ASSET_SLOTS> bootstrapSlots;
    bootstrapSlots.clear();

    SlotInfo slotInfo[MAX_ASSET_SLOTS];
    bzero(slotInfo);

    for (unsigned i = 0; i != count; ++i) {
        AssetGroup group;
        map.translate(vec[i], group);
        AssetSlot slot(vec[i].slot);

        ASSERT(numAssetSlots <= MAX_ASSET_SLOTS);
        if (slot.sys >= numAssetSlots) {
            LOG("LAUNCHER: Bootstrap group has invalid slot ID %d\n", slot.sys);
            return;
        }

        auto &info = slotInfo[slot.sys];
        bootstrapSlots.mark(slot.sys);

        if (group.isInstalled(cubes)) {
            installedGroups.mark(i);
            LOG("LAUNCHER: Bootstrap asset group %P already installed\n",
                group.sysHeader());
        } else {
            info.uninstalledBytes += group.compressedSize();
            info.uninstalledTiles += group.tileAllocation();
            needAnyInstall = true;
        }

        info.totalBytes += group.compressedSize();
        info.totalTiles += group.tileAllocation();
    }

    if (!needAnyInstall) {
        // All groups already installed. We're done!
        return;
    }

    /*
     * Second pass, clear asset slots.
     *
     * At this point, we know which slots won't be able to hold the
     * required assets without being erased first. Erase them
     * all up-front if necessary.
     *
     * At this point we can also calculate a total estimate for how
     * much compressed data we'll be sending.
     */

    unsigned totalBytes = 0;

    for (unsigned slot : bootstrapSlots) {
        auto &info = slotInfo[slot];

        if (info.totalTiles > TILES_PER_ASSET_SLOT) {
            LOG("LAUNCHER: Bootstrap groups in slot %d are too large (%d tiles)\n",
                slot, info.totalTiles);
            return;
        }

        if (info.uninstalledTiles == 0 || info.uninstalledTiles < AssetSlot(slot).tilesFree()) {
            // Everything fits in this slot!
            totalBytes += info.uninstalledBytes;
            continue;
        }

        LOG("LAUNCHER: Erasing asset slot %d\n", slot);
        AssetSlot(slot).erase();

        /*
         * If we had to erase any slots, disregard any prior knowledge of which
         * groups were already installed. (We don't know which groups were just erased!)
         *
         * Instead, we'll try to install everything, and rely on the asset loader to detect
         * already-installed groups during start().
         */
        installedGroups.clear();

        // Nothing is installed in this slot now, we'll need to install everything.
        totalBytes += info.totalBytes;
    }

    /*
     * Final pass: Asset loading!
     *
     * As we load each group, calculate a global progress estimate for all groups.
     */

    progress.begin(cubes);
    ScopedAssetLoader loader;

    unsigned previousBytes = 0;
    for (unsigned i = 0; i != count; ++i) {

        // Skip known-installed groups
        if (installedGroups.test(i))
            continue;

        AssetGroup group;
        map.translate(vec[i], group);
        AssetSlot slot(vec[i].slot);

        LOG("LAUNCHER: Bootstrapping asset group %P in slot %d\n",
            group.sysHeader(), slot.sys);

        if (!loader.start(group, slot, cubes)) {
            LOG("LAUNCHER: Asset loading failed!\n");
            continue;
        }

        while (!loader.isComplete()) {
            /*
             * Calculate a new progress value, using the average number of
             * bytes sent to all cubes participating in this load.
             */

            unsigned totalBytes = 0;
            unsigned totalCubes = 0;
            for (CubeID cube : cubes) {
                totalBytes += loader.progressBytes(cube);
                totalCubes++;
            }
            ASSERT(totalCubes != 0);

            unsigned averageBytes = totalBytes / totalCubes;
            unsigned percent = (previousBytes + averageBytes) * 100 / totalBytes;

            progress.paint(cubes, percent);
            System::paint();
        }

        loader.finish();
        previousBytes += group.compressedSize();
    }

    progress.end(cubes);
}
