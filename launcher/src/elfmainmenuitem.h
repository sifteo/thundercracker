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

    virtual void getAssets(Sifteo::MenuItem &assets);
    virtual void bootstrap(Sifteo::CubeSet cubes, ProgressDelegate &progress);

    virtual void exec() {
        volume.exec();
    }

    virtual CubeRange getCubeRange() {
        return cubeRange;
    }

    /// Look for all games on the system, and add them to the MainMenu.
    static void findGames(MainMenu &menu);

    /// See if we can automatically execute a single game. (Simulator only)
    static void autoexec();

private:
    /**
     * Max number of ELF main menu items. This is mostly dictated by the system's
     * limit on number of AssetGroups per AssetSlot, which is currently 24.
     */
    static const unsigned MAX_INSTANCES = 24;

    /// How many asset slots can one app use?
    static const unsigned MAX_ASSET_SLOTS = 4;

    /// How big is an empty asset slot?
    static const unsigned TILES_PER_ASSET_SLOT = 4096;

    /// Max number of bootstrap asset groups (Limited by max size of metadata values)
    static const unsigned MAX_BOOTSTRAP_GROUPS = 32;

    struct SlotInfo {
        unsigned totalBytes;
        unsigned totalTiles;
        unsigned uninstalledBytes;
        unsigned uninstalledTiles;
    };

    Sifteo::Volume volume;
    CubeRange cubeRange;
    uint8_t numAssetSlots;

    /**
     * Initialize from a Volume. Returns 'true' if we can successfully create
     * an ELFMainMenuItem for the volume, or 'false' if it should not appear
     * on the main menu.
     */
    bool init(Sifteo::Volume volume);

    static ELFMainMenuItem instances[MAX_INSTANCES];
};
