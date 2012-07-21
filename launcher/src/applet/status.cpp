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


static const unsigned kNumBatteryTiles = 3;
static const unsigned kNumBatteryLevels = 8 * kNumBatteryTiles;
static const unsigned kMaxBatteryLevel = 256;

template<typename T>
static void drawBattery(T &canvas, float batteryLevel, int levelCounter, Int2 pos)
{
    unsigned numBatteryLevels = batteryLevel * float(kNumBatteryLevels);
    numBatteryLevels = MIN(numBatteryLevels, levelCounter);
        
    for (int i = 0; i < kNumBatteryTiles; ++i) {
        unsigned baseBars = i * 8;
        if (numBatteryLevels > baseBars) {
            int numBars = numBatteryLevels - baseBars;
            canvas.image(vec(Battery_Black.tileWidth() * i + pos.x, pos.y), Battery_Black, numBars - 1);
        }
    }
}


MainMenuItem::Flags StatusApplet::getAssets(MenuItem &assets, MappedVolume&)
{
    icon.init();
    icon.image(vec(0,0), Icon_BatteryMaster);

    float batteryLevelBuddy = float(CubeID(0).batteryLevel()) / float(kMaxBatteryLevel); // TODO: Find out which CubeId we are dealing with in the menu
    batteryLevelBuddy = 0.4f; // XXX: test code
    drawBattery(icon, batteryLevelBuddy, levelCounter, vec(2, 2));

    float batteryLevelMaster = float(CubeID(0).batteryLevel()) / float(kMaxBatteryLevel); // TODO: use API for master battery
    batteryLevelMaster = 0.9f; // XXX: test code
    drawBattery(icon, batteryLevelMaster, levelCounter, vec(2, 8));

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
            vid.initMode(BG0);
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

            float batteryLevelBuddy = float(cube.batteryLevel()) / float(kMaxBatteryLevel);
            batteryLevelBuddy = 1.0f; // XXX: test code
            
            drawBattery(vid.bg0, batteryLevelBuddy, levelCounter, vec(4, 4));
        }
    }
}

void StatusApplet::add(MainMenu &menu)
{
    static StatusApplet instance;
    menu.append(&instance);
}
