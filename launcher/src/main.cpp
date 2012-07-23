/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "mainmenu.h"
#include "elfmainmenuitem.h"
#include "assets.gen.h"
#include "applet/getgames.h"
#include "shared.h"

using namespace Sifteo;

#ifndef SDK_VERSION
#error No SDK_VERSION is defined. Build system bug?
#endif

static Metadata M = Metadata()
    .title("System Launcher")
    .package("com.sifteo.launcher", TOSTRING(SDK_VERSION));


void connect(void*, unsigned cid)
{
    AudioTracker::play(UISound_ConnectBase);
    Shared::video[cid].attach(cid);
}

void disconnect(void*, unsigned cid)
{
    AudioTracker::play(UISound_CubeLost);
}

void main()
{
    // In simulation, if exactly one game is installed, run it immediately.
    ELFMainMenuItem::autoexec();

    AudioTracker::play(UISound_Startup);

    /// XXX: Big hack for testing... just wait for the first cube.
    while (CubeSet::connected().empty())
        System::yield();

    Events::cubeConnect.set(connect);
    Events::cubeDisconnect.set(disconnect);

    while (1) {
        static MainMenu menu;
        menu.init();

        // Populate the menu
        ELFMainMenuItem::findGames(menu);
        GetGamesApplet::add(menu);

        menu.run();
    }
}
