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

MainMenuItem::Flags StatusApplet::getAssets(MenuItem &assets, MappedVolume&)
{
    assets.icon = &Icon_BatteryMaster;
    return NONE;
}

void StatusApplet::exec()
{
}

void StatusApplet::arrive(CubeSet cubes, CubeID mainCube)
{
    for (CubeID cube : cubes)
        if (cube != mainCube) {
            auto &vid = Shared::video[cube];
            vid.bg0.image(vec(2, 2), Icon_Battery);
        }
}

void StatusApplet::depart(CubeSet cubes, CubeID mainCube)
{
    // Display a background on all other cubes
    for (CubeID cube : cubes)
        if (cube != mainCube) {
            auto &vid = Shared::video[cube];
            vid.initMode(BG0_SPR_BG1);
            vid.attach(cube);
            vid.bg0.erase(Menu_StripeTile);
        }
}

void StatusApplet::prepaint(CubeSet cubes, CubeID mainCube)
{
    for (CubeID cube : cubes)
        if (cube != mainCube) {
            float batteryLevelBuddy = float(cube.batteryLevel()) / float(kMaxBatteryLevel);
            //batteryLevelBuddy = 0.4f;
            
            unsigned batteryIconIndex = unsigned(batteryLevelBuddy * float(arraysize(BuddyBatteries) - 1));
            ASSERT(batteryIconIndex < arraysize(BuddyBatteries));

            auto &vid = Shared::video[cube];
            vid.sprites[0].setImage(BuddyBatteries[batteryIconIndex]);
            vid.sprites[0].move(vec(32, 40));
        } else {
            
            auto &vid = Shared::video[cube];
            
            float batteryLevelBuddy = float(CubeID(0).batteryLevel()) / float(kMaxBatteryLevel); // TODO: Find out which CubeId we are dealing with in the menu
            //batteryLevelBuddy = 0.4f;
            
            unsigned batteryIndexBuddy = unsigned(batteryLevelBuddy * float(arraysize(BuddyBatteries) - 1));
            ASSERT(batteryIndexBuddy < arraysize(BuddyBatteries));
            vid.sprites[0].setImage(BuddyBatteries[batteryIndexBuddy]);
            vid.sprites[0].move(vec(32, 32));

            float batteryLevelMaster = float(CubeID(0).batteryLevel()) / float(kMaxBatteryLevel); // TODO: use API for master battery
            //batteryLevelMaster = 0.8f;

            unsigned batteryIndexMaster = unsigned(batteryLevelMaster * float(arraysize(MasterBatteries) - 1));
            ASSERT(batteryIndexMaster < arraysize(MasterBatteries));
            vid.sprites[1].setImage(MasterBatteries[batteryIndexMaster]);
            vid.sprites[1].move(vec(32, 64));
        }
}

void StatusApplet::add(MainMenu &menu)
{
    static StatusApplet instance;
    menu.append(&instance);
}
