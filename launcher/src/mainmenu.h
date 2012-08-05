/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#include <sifteo.h>
#include <sifteo/menu.h>
#include "shared.h"
#include "defaultloadinganimation.h"
#include "mainmenuitem.h"


/**
 * The Main Menu is the menu which can run applications in the Launcher.
 *
 * It differs from other menus in that it supports asset bootstrapping,
 * via the MainMenuItem abstract base class.
 */

class MainMenu
{
public:

    /**
     * Set up the Main Menu's data structures, and empty its lists of items.
     */
    void init();

    /**
     * Append a new item to the internal list of menu items. The MainMenuItem
     * object must remain valid until the menu is run.
     */
    void append(MainMenuItem *item) {
        items.append(item);
    }

    /**
     * Execute the menu's main loop. When an item is chosen, it is exec()'ed.
     * If exec() returns, this function will return.
     */
    void run();

private:
    Sifteo::SystemTime cubeJoinTimestamps[CUBE_ALLOCATION];
    Sifteo::SystemTime connectSfxDelayTimestamp;
    Sifteo::SystemTime time;
    Sifteo::CubeID mainCube;
    Sifteo::CubeSet connectingCubes;    // Displaying logo OR still loading
    Sifteo::CubeSet loadingCubes;       // Cubes *displaying* a loading animation

    Sifteo::Menu menu;
    Sifteo::Array<MainMenuItem*, Shared::MAX_ITEMS> items;
    Sifteo::MenuItem menuItems[Shared::MAX_ITEMS + 1];
    int itemIndexCurrent;
    int itemIndexChoice;

    static const Sifteo::MenuAssets menuAssets;

    /**
     * A icon that we swap in if the user tries to launch a game that requires
     * an incompatible number of cubes
     */
    MainMenuItem::IconBuffer cubeRangeAlertIcon;
    const Sifteo::AssetImage *cubeRangeSavedIcon;

    /**
     * We hold onto our AssetLoader and AssetConfiguration persistently,
     * since asset loading may happen at any time while the menu is running.
     */
    Sifteo::AssetLoader loader;
    Shared::AssetConfiguration menuAssetConfig;
    DefaultLoadingAnimation loadingAnimation;

    void cubeConnect(unsigned cid);
    void cubeDisconnect(unsigned cid);
    void waitForACube();

    void updateSound();
    void updateMusic();
    void updateConnecting();

    void prepareAssets();
    
    bool canLaunchItem(unsigned index);
    void toggleCubeRangeAlert(unsigned index);
    void updateAlerts();

    void handleEvent(Sifteo::MenuEvent &e);

    // Note: these functions are marked NOINLINE as a cache usage optimization.
    NOINLINE void eventLoop();
    NOINLINE void execItem(unsigned index);
    NOINLINE void arriveItem(unsigned index);
    NOINLINE void departItem(unsigned index);
};
