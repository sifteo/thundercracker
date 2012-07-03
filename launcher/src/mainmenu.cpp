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
    // XXX: Fake cubeset initialization, until we have real cube connect/disconnect
    cubes = CubeSet(0,3);
    _SYS_enableCubes(cubes);

    // Load assets on all cubes
    loadAssets(cubes);

    // Pick one cube to be the 'main' cube, where we show the menu
    mainCube = *cubes.begin();

    // Display a background on all other cubes
    for (CubeID cube : cubes) {
        if (cube == mainCube)
            continue;

        auto& vid = Shared::video[cube];
        vid.initMode(BG0);
        vid.attach(cube);
        vid.bg0.erase(Menu_StripeTile);
    }
}

void MainMenu::append(MainMenuItem *item)
{
    items.append(item);
}

void MainMenu::run()
{
    Menu m(Shared::video[mainCube], &menuAssets, &menuItems[0]);

    m.setIconYOffset(8);

    struct MenuEvent e;
    while (m.pollEvent(&e)) {

        switch(e.type) {
            case MENU_ITEM_PRESS:
                
            default:
                break;
        }

    }
}

void MainMenu::loadAssets(Sifteo::CubeSet cubes)
{
    ScopedAssetLoader loader;

    // Bind the local volume's slots.
    _SYS_asset_bindSlots(Volume::running(), Shared::NUM_SLOTS);

    if (!loader.start(MenuGroup, Shared::primarySlot, cubes)) {
        Shared::primarySlot.erase();
        loader.start(MenuGroup, Shared::primarySlot, cubes);
    }

    if (loader.isComplete())
        return;

    DefaultLoadingAnimation anim;
    anim.begin(cubes);

    while (!loader.isComplete()) {

        // Report progress individually for each cube
        for (CubeID cube : cubes)
            anim.paint(CubeSet(cube), loader.progress(cube));

        System::paint();
    }

    anim.end(cubes);
}
