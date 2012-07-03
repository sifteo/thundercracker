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


void MainMenu::init()
{
    // XXX: Fake cubeset initialization, until we have real cube connect/disconnect
    cubes = CubeSet(0,3);
    _SYS_enableCubes(cubes);

    loadAssets(cubes);
}

void MainMenu::append(MainMenuItem *item)
{
    items.append(item);
}

void MainMenu::run()
{
    while (1) {
        System::paint();
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
