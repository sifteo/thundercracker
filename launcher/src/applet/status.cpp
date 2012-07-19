/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "applet/status.h"
#include "mainmenu.h"
#include "shared.h"
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

void StatusApplet::arrive(Sifteo::CubeSet cubes, Sifteo::CubeID mainCube)
{
    for (CubeID cube : cubes)
        if (cube != mainCube) {
            float batteryLevelBuddy = float(cube.batteryLevel()) / float(kMaxBatteryLevel);
            //batteryLevelBuddy = 0.4f;
            
            unsigned batteryIconIndex = unsigned(batteryLevelBuddy * float(arraysize(Icons_Battery) - 1));
            ASSERT(batteryIconIndex < arraysize(Icons_Battery));

            auto& vid = Shared::video[cube];
            vid.bg0.image(vec(2, 2), Icons_Battery[batteryIconIndex]);
        }
}

void StatusApplet::depart(Sifteo::CubeSet cubes, Sifteo::CubeID mainCube)
{
    // Display a background on all other cubes
    for (CubeID cube : cubes)
        if (cube != mainCube) {
            auto& vid = Shared::video[cube];
            vid.initMode(BG0);
            vid.attach(cube);
            vid.bg0.erase(Menu_StripeTile);
        }
}

void StatusApplet::add(MainMenu &menu)
{
    static StatusApplet instance;
    menu.append(&instance);
}
