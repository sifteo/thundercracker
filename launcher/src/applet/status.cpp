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

static const unsigned kNumBatteryLevels = 4;
static const unsigned kMaxBatteryLevel = 256;

MainMenuItem::Flags StatusApplet::getAssets(MenuItem &assets, MappedVolume&)
{
    icon.init();
    icon.image(vec(0,0), Icon_BatteryMaster);

    float batteryLevelBuddy = float(CubeID(0).batteryLevel()) / float(kMaxBatteryLevel); // TODO: Find out which CubeId we are dealing with in the menu
    batteryLevelBuddy = 1.0f;

    unsigned numBatteryLevelsBuddy = batteryLevelBuddy * float(kNumBatteryLevels);
    numBatteryLevelsBuddy = MIN(numBatteryLevelsBuddy, levelCounter);

    for (int i = 0; i < numBatteryLevelsBuddy; ++i)
    {
        icon.image(vec(Battery_Black.tileWidth() * i + 2, 3), Battery_Black);
    }

    float batteryLevelMaster = float(CubeID(0).batteryLevel()) / float(kMaxBatteryLevel); // TODO: use API for master battery
    batteryLevelMaster = 1.0f;

    unsigned numBatteryLevelsMaster = batteryLevelMaster * float(kNumBatteryLevels);
    numBatteryLevelsMaster = MIN(numBatteryLevelsMaster, levelCounter);

    for (int i = 0; i < numBatteryLevelsMaster; ++i)
    {
        icon.image(vec(Battery_Red.tileWidth() * i + 2, 7), Battery_Red);
    }
    
    assets.icon = icon;
    return NONE;
}

void StatusApplet::exec()
{
}

void StatusApplet::arrive(CubeSet cubes, CubeID mainCube)
{
    levelCounter = 0;

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
    static bool frame = true;
    if (frame) {
        if (levelCounter < kNumBatteryLevels)
        {
            ++levelCounter;
        }
    }
    frame = !frame;

    for (CubeID cube : cubes) {
        if (cube != mainCube) {
            auto &vid = Shared::video[cube];

            vid.bg0.image(vec(2,2), Icon_Battery);

            float batteryLevelBuddy = float(cube.batteryLevel()) / float(kMaxBatteryLevel); // TODO: Find out which CubeId we are dealing with in the menu
            batteryLevelBuddy = 1.0f;

            unsigned numBatteryLevelsBuddy = batteryLevelBuddy * float(kNumBatteryLevels);
            numBatteryLevelsBuddy = MIN(numBatteryLevelsBuddy, levelCounter);

            for (int i = 0; i < numBatteryLevelsBuddy; ++i)
            {
                vid.bg0.image(vec(Battery_Black.tileWidth() * i + 4, 5), Battery_Black);
            }

        }
    }
}

void StatusApplet::add(MainMenu &menu)
{
    static StatusApplet instance;
    menu.append(&instance);
}
