/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "applet/status.h"
#include "mainmenu.h"
#include "assets.gen.h"
#include <sifteo.h>
using namespace Sifteo;


MainMenuItem::Flags StatusApplet::getAssets(Sifteo::MenuItem &assets, Sifteo::MappedVolume&)
{
    assets.icon = &Icon_StatusMaster;
    return NONE;
}

void StatusApplet::exec()
{
}

void StatusApplet::add(MainMenu &menu)
{
    static StatusApplet instance;
    menu.append(&instance);
}
