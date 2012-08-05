/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "shared.h"
#include "mainmenu.h"
#include "mainmenuitem.h"
#include "defaultloadinganimation.h"
#include "assets.gen.h"
#include <sifteo.h>
#include <sifteo/menu.h>

using namespace Sifteo;

static const unsigned kFastClickAccelThreshold = 46;
static const unsigned kClickSpeedNormal = 200;
static const unsigned kClickSpeedFast = 300;

static const unsigned kConnectSfxDelayMS = 2000;
static const unsigned kDisplayBlueLogoTimeMS = 2000;
static const unsigned kNoCubesConnectedSfxMS = 5000;


static void drawText(MainMenuItem::IconBuffer &icon, const char *text, Int2 pos)
{
    for (int i = 0; text[i] != 0; ++i) {
        icon.image(vec(pos.x + i, pos.y), Font, uint8_t(text[i] - ' '));
    }
}

const MenuAssets MainMenu::menuAssets = {
    &Menu_BgTile, &Menu_Footer, NULL, {&Menu_Tip0, &Menu_Tip1, &Menu_Tip2, NULL}
};

void MainMenu::init()
{
    // Play the startup sound as early as possible
    AudioTracker::play(Tracker_Startup);

    Events::cubeConnect.set(&MainMenu::cubeConnect, this);
    Events::cubeDisconnect.set(&MainMenu::cubeDisconnect, this);

    loader.init();

    bzero(cubeJoinTimestamps);
    time = SystemTime::now();
    connectSfxDelayTimestamp = time;

    items.clear();
    itemIndexCurrent = 0;
    cubeRangeSavedIcon = NULL;

    _SYS_setCubeRange(1, _SYS_NUM_CUBE_SLOTS);
    _SYS_asset_bindSlots(Volume::running(), Shared::NUM_SLOTS);
}

void MainMenu::run()
{
    // Using our now-final list of items, build our AssetConfiguration
    prepareAssets();

    // Connect initial cubes
    for (CubeID cube : CubeSet::connected())
        cubeConnect(cube);

    // Find a default item, based on whatever volume was running last    
    if (Volume::previous() != Volume(0)) {
        for (unsigned i = 0, e = items.count(); i != e; ++i) {
            ASSERT(items[i] != NULL);
            if (items[i]->getVolume() == Volume::previous()) {
                itemIndexCurrent = i;
                break;
            }
        }
    }

    // Start with the menu on no cube. We'll update this during the main loop.
    mainCube = CubeID();

    eventLoop();
}

void MainMenu::eventLoop()
{
    while (1) {
        // Need to bind the menu to a new cube?
        if (!mainCube.isDefined()) {

            // Make sure we have at least one cube
            waitForACube();

            // Pick a new cube for the menu
            mainCube = *CubeSet::connected().begin();

            // (Re)initialize the menu on that cube
            menu.init(Shared::video[mainCube], &menuAssets, &menuItems[0]);
            menu.setIconYOffset(8);
            if (itemIndexCurrent >= 0)
                menu.anchor(itemIndexCurrent);
        }

        MenuEvent e;
        itemIndexChoice = -1;

        // Keep running until a choice is made or the menu cube disconnects
        while (mainCube.isDefined() && menu.pollEvent(&e)) {
            updateSound();
            updateMusic();
            updateAlerts();
            handleEvent(e);
        }

        if (itemIndexChoice >= 0)
            return execItem(itemIndexChoice);
    }
}

void MainMenu::waitForACube()
{
    SystemTime sfxTime = SystemTime::now();
    while (CubeSet::connected().empty()) {
        if ((SystemTime::now() - sfxTime).milliseconds() >= kNoCubesConnectedSfxMS) {
            sfxTime = SystemTime::now();
            AudioChannel(0).play(Sound_NoCubesConnected);
        }
        System::yield();
    }
}

void MainMenu::handleEvent(MenuEvent &e)
{
    bool performDefault = true;

    switch (e.type) {

        case MENU_ITEM_PRESS:
            if (canLaunchItem(e.item)) {
                AudioChannel(0).play(Sound_ConfirmClick);
                itemIndexChoice = e.item;
            } else {
                ASSERT(e.item < arraysize(items));
                if (items[e.item]->getCubeRange().isEmpty()) {
                    AudioChannel(0).play(Sound_NonPossibleAction);
                    performDefault = false;
                } else {
                    toggleCubeRangeAlert(e.item);
                    performDefault = false;
                }
            }
            break;

        case MENU_ITEM_ARRIVE:
            itemIndexCurrent = e.item;
            if (itemIndexCurrent >= 0) {
                arriveItem(itemIndexCurrent);
            }
            break;

        case MENU_ITEM_DEPART:
            if (itemIndexCurrent >= 0) {
                if (cubeRangeSavedIcon != NULL) {
                    menu.replaceIcon(itemIndexCurrent, cubeRangeSavedIcon);
                    cubeRangeSavedIcon = NULL;
                }
                departItem(itemIndexCurrent);
            }
            itemIndexCurrent = -1;
            break;

        default:
            break;

    }

    if (performDefault)
        menu.performDefault();
}

void MainMenu::cubeConnect(unsigned cid)
{
    SystemTime now = SystemTime::now();

    // Only play a connect sfx if it's been a while since the last one (or since boot)
    if ((now - connectSfxDelayTimestamp).milliseconds() >= kConnectSfxDelayMS) {
        connectSfxDelayTimestamp = now;
        AudioTracker::play(Tracker_CubeConnect);
    }

    // Reset this cube's connection timestamp. We won't use it until it has shown the logo for a while.
    cubeJoinTimestamps[cid] = now;

    // Attach buffers
    Shared::motion[cid].attach(cid);
    Shared::video[cid].attach(cid);

    // Initialize to blue Sifteo logo to match firmware.
    Shared::video[cid].initMode(BG0_ROM);
    Shared::video[cid].bg0.setPanning(vec(0,0));
    Shared::video[cid].bg0.image(vec(0,0), Logo);
}

void MainMenu::cubeDisconnect(unsigned cid)
{
    AudioTracker::play(Tracker_CubeDisconnect);

    // Were we using this cube? Not any more.
    if (mainCube == cid)
        mainCube = CubeID();
}

void MainMenu::updateSound()
{
    Sifteo::TimeDelta dt = Sifteo::SystemTime::now() - time;
    
    if (menu.getState() == MENU_STATE_TILTING) {
        unsigned threshold = abs(Shared::video[mainCube].virtualAccel().x) > kFastClickAccelThreshold ? kClickSpeedNormal : kClickSpeedFast;
        if (dt.milliseconds() >= threshold) {
            time += dt;
            AudioChannel(0).play(Sound_TiltClick);
        }
    } else if (menu.getState() == MENU_STATE_INERTIA) {
        unsigned threshold = 400;
        if (dt.milliseconds() >= threshold) {
            time += dt;
            AudioChannel(0).play(Sound_TiltClick);
        }
    }
}

void MainMenu::updateMusic()
{
    /*
     * If no other music is playing, play our menu.
     * (This lets the startup sound finish if it's still going)
     */

    if (AudioTracker::isStopped())
        AudioTracker::play(Tracker_MenuMusic);
}

bool MainMenu::canLaunchItem(unsigned index)
{
    ASSERT(index < arraysize(items));
    MainMenuItem *item = items[index];

    unsigned minCubes = item->getCubeRange().sys.minCubes;
    unsigned maxCubes = item->getCubeRange().sys.maxCubes;

    unsigned numCubes = CubeSet::connected().count();
    return numCubes >= minCubes && numCubes <= maxCubes;
}

void MainMenu::toggleCubeRangeAlert(unsigned index)
{
    if (cubeRangeSavedIcon == NULL) {
        AudioChannel(0).play(Sound_NonPossibleAction);

        ASSERT(index < arraysize(items));
        MainMenuItem *item = items[index];
        
        cubeRangeSavedIcon = menuItems[index].icon;
        
        cubeRangeAlertIcon.init();
        cubeRangeAlertIcon.image(vec(0,0), Icon_MoreCubes);

        String<2> buffer;
        buffer << item->getCubeRange().sys.minCubes;
        drawText(
            cubeRangeAlertIcon,
            buffer.c_str(),
            vec(item->getCubeRange().sys.minCubes < 10 ? 3 : 2, 3));
        
        unsigned numCubes = CubeSet::connected().count();
        unsigned numIcons = 12;
        
        if (item->getCubeRange().sys.maxCubes <= numIcons) {
            for (int i = 0; i < numIcons; ++i) {
                Int2 pos = vec((i % 4) * 2 + 2, (i / 4) * 2 + 6);
                if (i < numCubes) {
                    cubeRangeAlertIcon.image(pos, MoreCubesStates, 3);
                } else if (i < item->getCubeRange().sys.minCubes) {
                    cubeRangeAlertIcon.image(pos, MoreCubesStates, 2);
                } else if (i < item->getCubeRange().sys.maxCubes) {
                    cubeRangeAlertIcon.image(pos, MoreCubesStates, 1);
                } else {
                    cubeRangeAlertIcon.image(pos, MoreCubesStates, 0);
                }
            }
        }
        
        menu.replaceIcon(index, cubeRangeAlertIcon);
    } else {
        menu.replaceIcon(index, cubeRangeSavedIcon);
        cubeRangeSavedIcon = NULL;
    }
}

void MainMenu::updateAlerts()
{
    // XXX: Is this really needed here?
    auto& motion = Shared::motion[mainCube];
    motion.update();

    if (cubeRangeSavedIcon != NULL && itemIndexCurrent != -1) {
        if (motion.shake || motion.tilt.x != 0 || motion.tilt.y != 0) {
            toggleCubeRangeAlert(itemIndexCurrent);
        }
    }
}

void MainMenu::execItem(unsigned index)
{
    DefaultLoadingAnimation anim;

    ASSERT(index < arraysize(items));
    MainMenuItem *item = items[index];

    item->getCubeRange().set();
    item->bootstrap(CubeSet::connected(), anim);
    item->exec();
}

void MainMenu::arriveItem(unsigned index)
{
    ASSERT(index < arraysize(items));
    MainMenuItem *item = items[index];
    item->arrive(menu, index);
}

void MainMenu::departItem(unsigned index)
{
    ASSERT(index < arraysize(items));
    MainMenuItem *item = items[index];
    item->depart(menu, index);
}

void MainMenu::prepareAssets()
{
    /*
     * Gather loadable asset information from the MenuItems, and generate
     * an AssetConfiguration array.
     */

    ASSERT(menuAssetConfig.empty());

    // Add our local AssetGroup to the config
    menuAssetConfig.append(Shared::primarySlot, MenuGroup);

    // Clear the whole menuItems list. This zeroes the items we're
    // about to fill in, and it NULL-terminates the list.
    bzero(menuItems);

    // Set up each menu item
    for (unsigned I = 0, E = items.count(); I != E; ++I) {
        items[I]->getAssets(menuItems[I], menuAssetConfig);
    }
}
