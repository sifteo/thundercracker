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
    bool startupXmModHasFinished;
    int trackerVolume;
    static const int kTrackerVolumeNormal = 64;
    static const int kTrackerVolumeDucked = kTrackerVolumeNormal / 2;
    static const unsigned kUIResponseSoundChannel = 0;
    static const unsigned kConnectSoundChannel = 1;

    Sifteo::SystemTime time;
    Sifteo::CubeID mainCube;
    Sifteo::CubeSet connectingCubes;    // Displaying logo OR still loading
    Sifteo::CubeSet loadingCubes;       // Cubes *displaying* a loading animation

    Sifteo::Menu menu;
    Sifteo::Array<MainMenuItem*, Shared::MAX_ITEMS> items;
    Sifteo::MenuItem menuItems[Shared::MAX_ITEMS + 1];
    int itemIndexCurrent;
    int itemIndexChoice;
    int numGames;

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

    // Set up the Main Menu's data structures, and empty its lists of items.
    void init();
    void initMenu(unsigned initialIndex, bool popUp);

    // event handlers
    void volumeChanged(unsigned volumeHandle);
    void cubeConnect(unsigned cid);
    void cubeDisconnect(unsigned cid);
    void cubeTouch(unsigned cid);
    void neighborAdded(unsigned firstID, unsigned firstSide,
                       unsigned secondID, unsigned secondSide);
    void gameMenuEvent();
    void onBatteryLevelChange(unsigned cid);

    void waitForACube();

    void updateSound();
    void updateMusic();
    void updateConnecting();

    void prepareAssets();
    
    bool areEnoughCubesConnected(unsigned index);
    void updateCubeRangeAlert();
    void toggleCubeRangeAlert();
    void updateAlerts();

    void handleEvent(Sifteo::MenuEvent &e);

    // Note: these functions are marked NOINLINE as a cache usage optimization.
    NOINLINE void eventLoop();
    NOINLINE void execItem(unsigned index);
    NOINLINE void arriveItem(unsigned index);
    NOINLINE void departItem(unsigned index);
    NOINLINE void paint(unsigned index);
};
