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


const MenuAssets MainMenu::menuAssets = {
    &Menu_BgTile, &Menu_Footer, NULL, {&Menu_Tip0, &Menu_Tip1, &Menu_Tip2, NULL}
};


void MainMenu::init()
{
    // Empty our arrays
    items.clear();
    menuItems.clear();

    // XXX: Fake cubeset initialization, until we have real cube connect/disconnect
    cubes = CubeSet(0,3);
    _SYS_enableCubes(cubes);
}

void MainMenu::run()
{
    // Load our own local assets plus all icon assets
    loadAssets();

    // Pick one cube to be the 'main' cube, where we show the menu
    Sifteo::CubeID mainCube = *cubes.begin();

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
        switch(e.type) {

            case MENU_ITEM_PRESS:
                execItem(e.item);
                return;

            default:
                break;
        }
    }
}

void MainMenu::execItem(unsigned index)
{
    /// XXX: Instead of a separate animation, integrate this animation with the menu itself

    DefaultLoadingAnimation anim;

    ASSERT(index < arraysize(items));
    MainMenuItem *item = items[index];

    item->bootstrap(cubes, anim);
    item->exec();
}

void MainMenu::loadAssets()
{
    ScopedAssetLoader loader;
    DefaultLoadingAnimation anim;
    anim.begin(cubes);

    // Bind the local volume's slots.
    _SYS_asset_bindSlots(Volume::running(), Shared::NUM_SLOTS);

    // First, load our own assets into the primary slot
    if (!loader.start(MenuGroup, Shared::primarySlot, cubes)) {
        Shared::primarySlot.erase();
        loader.start(MenuGroup, Shared::primarySlot, cubes);
    }
    while (!loader.isComplete()) {
        // XXX: Report overall progress
        for (CubeID cube : cubes)
            anim.paint(CubeSet(cube), loader.progress(cube));
        System::paint();
    }

    // Now load each menu item into the secondary slot.
    // XXX: Report overall progress
    // XXX: Erase the slot first, if necessary
    //      (Both of these require asset group size accounting in the first pass)
    // XXX: Remember to only load things that need loading, i.e. not applet icons.

    for (MainMenuItem *item : items) {
        MappedVolume map;
        MenuItem &mi = menuItems.append();
        bzero(mi);
        item->getAssets(mi, map);

        loader.start(mi.icon->assetGroup(), Shared::secondarySlot, cubes);
        while (!loader.isComplete()) {
            for (CubeID cube : cubes)
                anim.paint(CubeSet(cube), loader.progress(cube));
            System::paint();
        }
    }

    anim.end(cubes);
}
