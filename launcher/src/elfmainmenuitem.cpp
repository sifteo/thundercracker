/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "elfmainmenuitem.h"
#include "mainmenu.h"
#include "assetloaderbypassdelegate.h"
#include "nineblock.h"
#include "assets.gen.h"
#include <sifteo.h>
#include <sifteo/asset.h>
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

    if (Volume::previous())
        return;

    Array<Volume, 2> volumes;
    Volume::list(Volume::T_GAME, volumes);
    if (volumes.count() != 1)
        return;

    ELFMainMenuItem &inst = instances[0];
    if (!inst.init(volumes[0]))
        return;

    LOG("LAUNCHER: Automatically executing single game\n");

    // Wait a little bit to allow all cube connection events to process
    while (SystemTime::now().uptimeMS() < 500)
        System::yield();

    AssetLoaderBypassDelegate delegate;
    inst.getCubeRange().set();
    inst.bootstrap(CubeSet::connected(), delegate);
    inst.exec();
}

void ELFMainMenuItem::findGames(Array<MainMenuItem*, Shared::MAX_ITEMS> &items)
{
    /*
     * Get the list of games from our filesystem. (Limited
     * to the max number of main menu items we can store)
     */

    Array<Volume, MAX_INSTANCES> volumes;
    Volume::list(Volume::T_GAME, volumes);

    items.clear();

    /*
     * Create an ELFMainMenuItem for each, skipping any volumes
     * that cause init() to return false.
     */

    unsigned volI = 0, itemI = 0;
    unsigned volE = volumes.count();

    while (volI != volE) {
        ELFMainMenuItem *inst = &instances[itemI];
        Volume vol = volumes[volI];

        if (inst->init(vol)) {
            items.append(inst);
            itemI++;
        }
        volI++;
    }
}

bool ELFMainMenuItem::init(Volume volume)
{
    /*
     * Load critical metadata from this volume into RAM, and check whether
     * it's suitable to include in the launcher menu. This loads only
     * lightweight resources from the volume. No assets are stored yet.
     */

    STATIC_ASSERT(MAX_INSTANCES < Shared::MAX_ITEMS);

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

    // Save the UUID
    uuid = *map.uuid();

    // See if there's a usable icon? We'll load it later, if so.
    hasValidIcon = checkIcon(map);

    return true;
}

bool ELFMainMenuItem::checkIcon(MappedVolume &map)
{
    // Validate the required icon, but don't save it yet.
    auto iconMeta = map.metadata<_SYSMetadataImage>(_SYS_METADATA_ICON_96x96);
    if (!iconMeta) {
        LOG("LAUNCHER: Warning, no 96x96 icon found.\n");
        return false;
    }
    if (iconMeta->width != icon.buffer.tileWidth() || iconMeta->height != icon.buffer.tileHeight()) {
        LOG("LAUNCHER: Warning, icon size is incorrect.\n");
        return false;
    }

    // Calculate how much space the icon's assets require
    AssetImage iconImage;
    AssetGroup iconGroup;
    map.translate(iconMeta, iconImage, iconGroup);
    unsigned tileAllocation = iconGroup.tileAllocation();

    // Check the size of this group. It should be no larger than the worst-case uncompressed size
    unsigned maxTileAllocation = roundup(icon.buffer.numTiles(), _SYS_ASSET_GROUP_SIZE_UNIT);
    if (tileAllocation > maxTileAllocation) {
        LOG("LAUNCHER: Warning, icon AssetGroup is too large. "
            "Make sure your icon is in an AssetGroup by itself! Found a "
            "group with %d tiles, expected no more than %d.",
            tileAllocation, maxTileAllocation);
        return false;
    }

    return true;
}

void ELFMainMenuItem::getAssets(Sifteo::MenuItem &menuItem, Shared::AssetConfiguration &config)
{
    if (hasValidIcon) {
        /*
         * Gather an icon from this volume.
         */

        MappedVolume map(volume);

        // We already validated the icon metadata
        auto iconMeta = map.metadata<_SYSMetadataImage>(_SYS_METADATA_ICON_96x96);
        ASSERT(iconMeta);
        ASSERT(iconMeta->width == icon.buffer.tileWidth());
        ASSERT(iconMeta->height == icon.buffer.tileHeight());

        // Mapping translation; convert to an AssetImage and AssetGroup.
        AssetImage iconSrc;
        map.translate(iconMeta, iconSrc, icon.group);

        // The above AssetImage still references data in the mapped volume,
        // which won't be available later. Copy / decompress it into RAM.
        icon.buffer.init();
        icon.buffer.image(vec(0,0), iconSrc);
        menuItem.icon = icon.buffer;

        // Remember to load this asset group later
        config.append(Shared::iconSlot, icon.group, volume);

    } else {
        /*
         * No icon? Create a randomly generated icon.
         */

        icon.buffer.init();
        NineBlock::generate(crc32(uuid), icon.buffer);
        menuItem.icon = icon.buffer;
    }
}

void ELFMainMenuItem::bootstrap(Sifteo::CubeSet cubes, ProgressDelegate &progress)
{
    /*
     * Set up this app's cubes and asset slots.
     *
     * After this point, we can't access any of the launcher's
     * AssetGroups without reverting to our own binding.
     *
     * Note that our VideoBuffer stay in the orignal A21 bank, for the
     * time being. They won't switch immediately after bindSlots; not
     * til the next time they're attach()'ed to a cube.
     */

    _SYS_asset_bindSlots(volume, numAssetSlots);

    /*
     * Bootstrap any number of asset groups from this volume.
     */

    MappedVolume map(volume);
    MappedVolume::BootstrapAssetGroups groups;
    MappedVolume::BootstrapAssetConfiguration config;
    map.getBootstrap(groups, config);

    if (config.empty()) {
        LOG(("LAUNCHER: No bootstrap assets found\n"));
        return;
    }

    progress.begin(cubes);
    ScopedAssetLoader loader;
    loader.start(config, cubes);

    Sifteo::SystemTime time = SystemTime::now();

    while (!loader.isComplete()) {
        // Play loading blip every so often
        Sifteo::TimeDelta dt = Sifteo::SystemTime::now() - time;
        if (dt.milliseconds() >= LOADING_SOUND_PERIOD) {
            time += dt;
            AudioChannel(0).play(Sound_LoadingGame);
        }

        progress.paint(cubes, loader.averageProgress(100));
        System::paint();
    }

    progress.end(cubes);
}
