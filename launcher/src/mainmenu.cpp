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


static unsigned getNumCubes(CubeSet cubes)
{
    unsigned count = 0;
    for (int i = 0; i < cubes.size(); ++i) {
        if (cubes.test(i))
            ++count;
    }
    return count;
}

const MenuAssets MainMenu::menuAssets = {
    &Menu_BgTile, &Menu_Footer, NULL, {&Menu_Tip0, &Menu_Tip1, &Menu_Tip2, NULL}
};


void MainMenu::init()
{
    items.clear();
    itemIndexCurrent = 0;
    cubeRangeSavedIcon = NULL;

    // XXX: Fake cubeset initialization, until we have real cube connect/disconnect
    cubes = CubeSet(0,3);
    _SYS_enableCubes(cubes);
}

void MainMenu::run()
{
    // Load our own local assets plus all icon assets
    loadAssets();

    // Pick one cube to be the 'main' cube, where we show the menu
    mainCube = *cubes.begin();

    // Display a background on all other cubes
    for (CubeID cube : cubes)
        if (cube != mainCube) {
            auto& vid = Shared::video[cube];
            vid.initMode(BG0);
            vid.attach(cube);
            vid.bg0.erase(Menu_StripeTile);
        }

    // Run the menu on our main cube
    Menu m(Shared::video[mainCube], &menuAssets, &menuItems[0]);
    m.setIconYOffset(8);
    eventLoop(m);
}

void MainMenu::eventLoop(Menu &m)
{
    struct MenuEvent e;
    while (m.pollEvent(&e)) {

        updateMusic();
        updateIcons(m);
        checkForAlertDismiss(m);

        bool performDefault = true;

        switch(e.type) {

            case MENU_ITEM_PRESS:
                if (canLaunchItem(e.item)) {
                    execItem(e.item);
                    return;
                } else {
                    toggleCubeRangeAlert(e.item, m);
                    performDefault = false;
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
                        m.replaceIcon(itemIndexCurrent, cubeRangeSavedIcon);
                        cubeRangeSavedIcon = NULL;
                    }
                    departItem(itemIndexCurrent);
                }
                itemIndexCurrent = -1;
                break;
            case MENU_PREPAINT:
                if (itemIndexCurrent >= 0) {
                    prepaintItem(itemIndexCurrent);
                }
                break;
            default:
                break;

        }

        if (performDefault)
            m.performDefault();
    }
}

void MainMenu::updateMusic()
{
    /*
     * If no other music is playing, play our menu.
     * (This lets the startup sound finish if it's still going)
     */

    if (AudioTracker::isStopped())
        AudioTracker::play(UISound_MenuMusic);
}

void MainMenu::updateIcons(Menu &menu)
{
    for (unsigned i = 0, e = items.count(); i != e; ++i) {
        if (items[i]->autoRefreshIcon()) {
            MenuItem &mi = menuItems[i];
            
            MappedVolume map;
            auto flags = items[i]->getAssets(mi, map);
            
            menu.replaceIcon(i, mi.icon);
        }
    }
}

bool MainMenu::canLaunchItem(unsigned index)
{
    ASSERT(index < arraysize(items));
    MainMenuItem *item = items[index];

    unsigned minCubes = item->getCubeRange().sys.minCubes;
    unsigned maxCubes = item->getCubeRange().sys.maxCubes;

    unsigned numCubes = getNumCubes(cubes);
    return numCubes >= minCubes && numCubes <= maxCubes;
}

void MainMenu::toggleCubeRangeAlert(unsigned index, Sifteo::Menu &menu)
{
    if (cubeRangeSavedIcon == NULL) {
        ASSERT(index < arraysize(items));
        MainMenuItem *item = items[index];
        
        cubeRangeSavedIcon = menuItems[index].icon;
        
        cubeRangeAlertIcon.init();
        cubeRangeAlertIcon.image(vec(0,0), Icon_CubeRange);
        cubeRangeAlertIcon.image(vec(2,4), Numbers, item->getCubeRange().sys.minCubes);
        cubeRangeAlertIcon.image(vec(7,4), Numbers, item->getCubeRange().sys.maxCubes);
        
        menu.replaceIcon(index, cubeRangeAlertIcon);
    } else {
        menu.replaceIcon(index, cubeRangeSavedIcon);
        cubeRangeSavedIcon = NULL;
    }
}

void MainMenu::checkForAlertDismiss(Sifteo::Menu &menu)
{
    if (cubeRangeSavedIcon != NULL && itemIndexCurrent != -1) {
        if (mainCube.isShaking() || mainCube.tilt().x != 0 || mainCube.tilt().y != 0) {
            toggleCubeRangeAlert(itemIndexCurrent, menu);
        }
    }
}

void MainMenu::execItem(unsigned index)
{
    /// XXX: Instead of a separate animation, integrate this animation with the menu itself
    /// XXX: Cube range init here is temporary.

    DefaultLoadingAnimation anim;

    MainMenuItem *item = items[index];
    CubeSet itemCubes = item->getCubeRange().initMinimum();
    item->bootstrap(itemCubes, anim);
    item->exec();
}

void MainMenu::arriveItem(unsigned index)
{
    ASSERT(index < arraysize(items));
    MainMenuItem *item = items[index];
    item->arrive(cubes, mainCube);
}

void MainMenu::departItem(unsigned index)
{
    ASSERT(index < arraysize(items));
    MainMenuItem *item = items[index];
    item->depart(cubes, mainCube);
}

void MainMenu::prepaintItem(unsigned index)
{
    ASSERT(index < arraysize(items));
    MainMenuItem *item = items[index];
    item->prepaint(cubes, mainCube);
}

void MainMenu::loadAssets()
{
    ScopedAssetLoader loader;
    DefaultLoadingAnimation anim;
    anim.begin(cubes);

    // Bind the local volume's slots.
    _SYS_asset_bindSlots(Volume::running(), Shared::NUM_SLOTS);

    /*
     * First, collect information about what we need to install.
     * We need to know (a) which asset groups to install, and
     * (b) whether the not-yet-installed ones will fit in our
     * slot without erasing it.
     *
     * Note that we can't really calculate this ahead-of-time,
     * since (a) depends on the state of the cubes we're using.
     *
     * To do this in a separate pass from the actual asset install,
     * we do need to map the volumes twice. Gather as much data
     * as possible now, so we can skip entire items below when
     * we know they won't require any asset loading.
     */

    unsigned uninstalledTiles = 0;
    unsigned uninstalledBytes = 0;
    unsigned totalBytes = 0;

    Volume mappedVolumes[MAX_ITEMS];

    BitArray<MAX_ITEMS> allLoadableItems;
    BitArray<MAX_ITEMS> uninstalledItems;
    allLoadableItems.clear();
    uninstalledItems.clear();

    // Clear the whole menuItems list. This zeroes the items we're
    // about to fill in, and it NULL-terminates the list.
    bzero(menuItems);

    for (unsigned I = 0, E = items.count(); I != E; ++I) {
        MainMenuItem *item = items[I];
        MappedVolume map;

        // Copy asset tiles into RAM, and map the volume
        MenuItem &mi = menuItems[I];
        auto flags = item->getAssets(mi, map);

        if (mi.icon && (flags & MainMenuItem::LOAD_ASSETS)) {
            AssetGroup &group = mi.icon->assetGroup();

            mappedVolumes[I] = map.volume();

            totalBytes += group.compressedSize();
            allLoadableItems.mark(I);

            if (!group.isInstalled(cubes)) {
                uninstalledTiles += group.tileAllocation();
                uninstalledBytes += group.compressedSize();
                uninstalledItems.mark(I);
            }
        }
    }

    /*
     * Now we can calculate whether the iconSlot needs to be erased,
     * and therefore whether we're installing just the not-yet-installed
     * icon assets, or all icon assets. This lets us calculate overall
     * progress for this whole series of loading operations.
     */

    if (Shared::iconSlot.tilesFree() < uninstalledTiles) {
        // Erase the slot. Now everything is uninstalled.
        Shared::iconSlot.erase();
        uninstalledBytes = totalBytes;
        uninstalledItems = allLoadableItems;
    }

    /*
     * First asset load: Put our own menu assets into the primary slot,
     * if they aren't already there. Odds are, they will be, but this
     * lets applets repurpose the primary slot if they need to.
     */

    // Keep count of progress from previous load operations.
    unsigned progress = 0;

    if (!MenuGroup.isInstalled(cubes)) {

        // Include the size of this group in our progress calculation
        uninstalledBytes += MenuGroup.compressedSize();

        if (!loader.start(MenuGroup, Shared::primarySlot, cubes)) {
            Shared::primarySlot.erase();
            loader.start(MenuGroup, Shared::primarySlot, cubes);
        }

        while (!loader.isComplete()) {
            for (CubeID cube : cubes) {
                anim.paint(CubeSet(cube), clamp<int>(loader.progressBytes(cube)
                    * 100 / uninstalledBytes, 0, 100));
            }
            System::paint();
        }

        loader.finish();
        progress += MenuGroup.compressedSize();
    }

    /*
     * Now we can loop over the individual items that may need asset installation,
     * and install each one while calculating total progress.
     */

    for (unsigned I : uninstalledItems) {
        MappedVolume map(mappedVolumes[I]);
        MenuItem &mi = menuItems[I];

        ASSERT(mi.icon);
        AssetGroup &group = mi.icon->assetGroup();

        loader.start(group, Shared::iconSlot, cubes);

        while (!loader.isComplete()) {
            for (CubeID cube : cubes) {
                anim.paint(CubeSet(cube), clamp<int>((progress + loader.progressBytes(cube))
                    * 100 / uninstalledBytes, 0, 100));
            }
            System::paint();
        }

        loader.finish();
        progress += group.compressedSize();
    }

    anim.end(cubes);
}
