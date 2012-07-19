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
	float batteryLevelBuddy = float(CubeID(0).batteryLevel()) / float(kMaxBatteryLevel); // TODO: Find out which CubeId we are dealing with in the menu
	float batteryLevelMaster = float(CubeID(0).batteryLevel()) / float(kMaxBatteryLevel); // TODO: use API for master battery

	//batteryLevelBuddy = 0.4f;
	//batteryLevelMaster = 0.8f;

	ASSERT(arraysize(Icons_BatteryMaster) / arraysize(Icons_Battery) == arraysize(Icons_Battery));
	unsigned batteryIndexBuddy = unsigned(batteryLevelBuddy * float(arraysize(Icons_Battery) - 1));
	unsigned batteryIndexMaster = unsigned(batteryLevelMaster * float(arraysize(Icons_Battery) - 1));
	unsigned batteryIconIndex = batteryIndexBuddy * arraysize(Icons_Battery) + batteryIndexMaster;

	ASSERT(batteryIconIndex < arraysize(Icons_BatteryMaster));
    assets.icon = &Icons_BatteryMaster[batteryIconIndex];
    
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
