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
static const unsigned kDisplayBlueLogoTimeMS = 2000;
static const unsigned kNoCubesConnectedSfxMS = 5000;


static void drawText(RelocatableTileBuffer<12,12> &icon, const char *text, Int2 pos)
{
    for (int i = 0; text[i] != 0; ++i) {
        icon.image(vec(pos.x + i, pos.y), Font, text[i]-32);
    }
}

const MenuAssets MainMenu::menuAssets = {
    &Menu_BgTile, &Menu_Footer, NULL, {&Menu_Tip0, &Menu_Tip1, &Menu_Tip2, NULL}
};

void MainMenu::init()
{
    Events::cubeConnect.set(&MainMenu::cubeConnect, this);
    Events::cubeDisconnect.set(&MainMenu::cubeDisconnect, this);

    loader.init();

    time = SystemTime::now();

    items.clear();
    itemIndexCurrent = 0;
    cubeRangeSavedIcon = NULL;

    menuDirty = false;
    
    _SYS_setCubeRange(1, _SYS_NUM_CUBE_SLOTS);
    _SYS_asset_bindSlots(Volume::running(), Shared::NUM_SLOTS);
}

void MainMenu::run()
{
    prepareAssets();
    waitForACube();
    
    // Initialize to blue Sifteo logo to match firmware.
    for (CubeID cube : CubeSet::connected()) {
        Shared::motion[cube].attach(cube);
        Shared::video[cube].attach(cube);
        Shared::video[cube].initMode(BG0_ROM);
        Shared::video[cube].bg0.image(vec(0,0), Logo);
    }
    System::paint();
    
    AudioTracker::play(Tracker_Startup);
    
    // Pick one cube to be the 'main' cube, where we show the menu
    mainCube = *cubes().begin();

    // Run the menu on our main cube
    Menu m(Shared::video[mainCube], &menuAssets, &menuItems[0]);
    m.setIconYOffset(8);
    
    if (Volume::previous() != Volume(0)) {
        for (unsigned i = 0, e = items.count(); i != e; ++i) {
            ASSERT(items[i] != NULL);
            if (items[i]->getVolume() == Volume::previous()) {
                m.anchor(i, true);
            }
        }
    }
    
    eventLoop(m);
}

void MainMenu::eventLoop(Menu &m)
{
    int itemChoice = -1;

    struct MenuEvent e;
    while (m.pollEvent(&e)) {

        waitForACube();
        updateAnchor(m);
        updateSound(m);
        updateMusic();
        updateAlerts(m);

        bool performDefault = true;

        switch(e.type) {

            case MENU_ITEM_PRESS:
                if (canLaunchItem(e.item)) {
                    AudioChannel(0).play(Sound_ConfirmClick);
                    itemChoice = e.item;
                } else {
                    ASSERT(e.item < arraysize(items));
                    if (items[e.item]->getCubeRange().isEmpty()) {
                        AudioChannel(0).play(Sound_NonPossibleAction);
                        performDefault = false;
                    } else {
                        toggleCubeRangeAlert(e.item, m);
                        performDefault = false;
                    }
                }
                break;
            case MENU_ITEM_ARRIVE:
                itemIndexCurrent = e.item;
                if (itemIndexCurrent >= 0) {
                    arriveItem(m, itemIndexCurrent);
                }
                break;
            case MENU_ITEM_DEPART:
                if (itemIndexCurrent >= 0) {
                    if (cubeRangeSavedIcon != NULL) {
                        m.replaceIcon(itemIndexCurrent, cubeRangeSavedIcon);
                        cubeRangeSavedIcon = NULL;
                    }
                    departItem(m, itemIndexCurrent);
                }
                itemIndexCurrent = -1;
                break;
            default:
                break;

        }

        if (performDefault)
            m.performDefault();
    }

    if (itemChoice != -1)
        execItem(itemChoice);
}

CubeSet MainMenu::cubes()
{
    return CubeSet::connected();
}

void MainMenu::cubeConnect(unsigned cid)
{
    AudioTracker::play(Tracker_CubeConnect);

    Shared::motion[cid].attach(cid);

    Shared::video[cid].attach(cid);
    Shared::video[cid].initMode(BG0_ROM);
    Shared::video[cid].bg0.setPanning(vec(0,0));
    Shared::video[cid].bg0.image(vec(0,0), Logo);

    loader.start(menuAssetConfig, CubeSet(cid));

    if (cid == mainCube) {
        menuDirty = true;
    }
}

void MainMenu::cubeDisconnect(unsigned cid)
{
    AudioTracker::play(Tracker_CubeDisconnect);
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

void MainMenu::updateAnchor(Menu &m)
{
    if (menuDirty) {
        m.reset();
        if (itemIndexCurrent != -1) {
            m.anchor(itemIndexCurrent);
        }
        menuDirty = false;
        System::paint();
    }
}

void MainMenu::updateSound(Sifteo::Menu &menu)
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

    unsigned numCubes = cubes().count();
    return numCubes >= minCubes && numCubes <= maxCubes;
}

void MainMenu::toggleCubeRangeAlert(unsigned index, Sifteo::Menu &menu)
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

void MainMenu::updateAlerts(Sifteo::Menu &menu)
{
    // XXX: Is this really needed here?
    auto& motion = Shared::motion[mainCube];
    motion.update();

    if (cubeRangeSavedIcon != NULL && itemIndexCurrent != -1) {
        if (motion.shake || motion.tilt.x != 0 || motion.tilt.y != 0) {
            toggleCubeRangeAlert(itemIndexCurrent, menu);
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

void MainMenu::arriveItem(Menu &menu, unsigned index)
{
    ASSERT(index < arraysize(items));
    MainMenuItem *item = items[index];
    item->arrive(menu, index);
}

void MainMenu::departItem(Menu &menu, unsigned index)
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
