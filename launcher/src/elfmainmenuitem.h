/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#include <sifteo.h>
#include "mainmenuitem.h"

class MainMenu;


/**
 * MainMenuItem subclass for menu items implemented by external ELF binaries.
 */
 
class ELFMainMenuItem : public MainMenuItem
{
public:
    virtual void getAssets(Sifteo::MenuItem &menuItem, Shared::AssetConfiguration &config);
    virtual void bootstrap(Sifteo::CubeSet cubes, ProgressDelegate &progress);

    virtual Sifteo::Volume getVolume() {
        return volume;
    }

    virtual void exec() {
        volume.exec();
    }

    virtual CubeRange getCubeRange() const {
        return cubeRange;
    }

    static bool firstRunPresent() {
        return firstRun != 0;
    }

    virtual bool isFirstRun() const {
        return firstRun == this;
    }

    /// Look for all games on the system, and add them to the MainMenu.
    static void findGames(Sifteo::Array<MainMenuItem*, Shared::MAX_ITEMS> &items);

    /// See if we can automatically execute a single game. (Simulator only)
    static void autoexec();

private:
    /**
     * Max number of ELF main menu items. This is mostly dictated by the system's
     * limit on number of AssetGroups per AssetSlot.
     */
    static const unsigned MAX_INSTANCES = _SYS_ASSET_GROUPS_PER_SLOT;

    /// How many asset slots can one app use?
    static const unsigned MAX_ASSET_SLOTS = _SYS_ASSET_SLOTS_PER_BANK;

    /// How big is an empty asset slot?
    static const unsigned TILES_PER_ASSET_SLOT = _SYS_TILES_PER_ASSETSLOT;

    /// Max number of bootstrap asset groups (Limited by max size of metadata values)
    static const unsigned MAX_BOOTSTRAP_GROUPS = _SYS_MAX_METADATA_ITEM_BYTES / sizeof(_SYSMetadataBootAsset);

    /// Period (in ms) for loading sound while bootstrapping a game.
    static const unsigned LOADING_SOUND_PERIOD = 333;

    static ELFMainMenuItem *firstRun;

    struct SlotInfo {
        unsigned totalBytes;
        unsigned totalTiles;
        unsigned uninstalledBytes;
        unsigned uninstalledTiles;
    };

    CubeRange cubeRange;
    uint8_t numAssetSlots;
    bool hasValidIcon;
    Sifteo::MappedVolume::UUID uuid;
    Sifteo::Volume volume;

    /*
     * Local storage for icon assets.
     *
     * The 'image' here stores an uncompressed copy, in RAM, of the icon's tile
     * indices. The 'group' references mapped AssetGroup data which isn't available
     * after we unmap the game's volume, but perhaps more importantly it stores
     * information about the load address of this icon's assets on each cube.
     */
    struct {
        IconBuffer buffer;
        Sifteo::AssetGroup group;
    } icon;

    /**
     * Initialize from a Volume. Returns 'true' if we can successfully create
     * an ELFMainMenuItem for the volume, or 'false' if it should not appear
     * on the main menu.
     */
    bool init(Sifteo::Volume volume, bool *outFirstRun=0);

    /**
     * Validate volume metadata that will be required later by getAssets()
     */
    bool checkIcon(Sifteo::MappedVolume &map);



    /**
     * Average bytes of asset loading progress across multiple cubes
     */
    static unsigned averageProgressBytes(const Sifteo::AssetLoader &loader, Sifteo::CubeSet cubes);

    static ELFMainMenuItem instances[MAX_INSTANCES];
};
