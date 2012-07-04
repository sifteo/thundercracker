/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "mainmenu.h"
#include "elfmainmenuitem.h"
#include "assets.gen.h"
#include "applet/getgames.h"

using namespace Sifteo;

static Metadata M = Metadata()
    .title("System Launcher")
    .package("com.sifteo.launcher", TOSTRING(SDK_VERSION));


void main()
{
    // In simulation, if exactly one game is installed, run it immediately.
    ELFMainMenuItem::autoexec();

    AudioTracker::play(UISound_Startup);

    while (1) {
        static MainMenu menu;
        menu.init();

        // Populate the menu
        ELFMainMenuItem::findGames(menu);
        GetGamesApplet::add(menu);

        menu.run();
    }
}
