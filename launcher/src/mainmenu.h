/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker launcher
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
    bool isBootstrapping;   // we've started bootstrapping a game's assets

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
    void initMenu(unsigned initialIndex, bool popUp, int panTarget=-1);

    // event handlers
    void volumeChanged(unsigned volumeHandle);
    void cubeConnect(unsigned cid);
    void cubeDisconnect(unsigned cid);
    void cubeTouch(unsigned cid);
    void onBatteryLevelChange(unsigned cid);

    void waitForACube();
    void checkForFirstRun();

    void updateSound();
    void updateFooter(unsigned itemIndex);
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
    NOINLINE void execItem(MainMenuItem *item, bool bootstrap = true);
    NOINLINE void arriveItem(unsigned index);
    NOINLINE void departItem(unsigned index);
    NOINLINE void paint(unsigned index);
};
