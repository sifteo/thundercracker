/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "applet/getgames.h"
#include "mainmenu.h"
#include "assets.gen.h"
#include <sifteo.h>
using namespace Sifteo;


void GetGamesApplet::getAssets(Sifteo::MenuItem &assets)
{
    assets.icon = &Icon_GetGames;
}

void GetGamesApplet::exec()
{
    LOG("XXX: Run the 'Get More Games' applet\n");
}

void GetGamesApplet::add(MainMenu &menu)
{
    static GetGamesApplet instance;
    menu.append(&instance);
}
