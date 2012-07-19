/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "applet/status.h"
#include "mainmenu.h"
#include "assets.gen.h"
#include <sifteo.h>
using namespace Sifteo;

static const unsigned kMaxBatteryLevel = 256;

MainMenuItem::Flags StatusApplet::getAssets(Sifteo::MenuItem &assets, Sifteo::MappedVolume&)
{
	// TODO: Find out which CubeId we are dealing with in the menu
	
	float batteryLevel = float(CubeId(0).batteryLevel) / float(kMaxBatteryLevel);
	unsigned batterIconIndex = unsigned(batteryLevel * float(arraysize(Icons_Battery) - 1));

	ASSERT(batterIconIndex < arraysize(Icons_Battery));
    assets.icon = &Icons_Battery[batterIconIndex];
    
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
