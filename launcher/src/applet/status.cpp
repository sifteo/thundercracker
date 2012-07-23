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

template<typename T>
static void drawBattery(T &canvas, float batteryLevel, int levelCounter, Int2 pos)
{
    unsigned numBatteryLevels = batteryLevel * float(BatteryBars.numFrames());
    numBatteryLevels = MIN(numBatteryLevels, levelCounter);
    
    canvas.image(vec(pos.x-1, pos.y-1), Battery);

    if (numBatteryLevels > 0) {
        --numBatteryLevels;
        ASSERT(numBatteryLevels < BatteryBars.numFrames());
        canvas.image(pos, BatteryBars, numBatteryLevels);
    }
}

template<typename T>
static void drawText(T &canvas, const char* text, Int2 pos)
{
    for (int i = 0; text[i] != 0; i++) {
        canvas.image(vec(pos.x + i, pos.y), Font, text[i]-32);
    }
}

static unsigned getNumCubes(CubeSet cubes)
{
    unsigned count = 0;
    for (int i = 0; i < cubes.size(); ++i) {
        if (cubes.test(i))
            ++count;
    }
    return count;
}

static unsigned getFreeBlocks()
{
    // TODO: amount of free blocks
    return 128;
}


MainMenuItem::Flags StatusApplet::getAssets(MenuItem &assets, MappedVolume &)
{
    icon.init();
    icon.image(vec(0,0), Icon_Battery);

    float batteryLevelBuddy = float(CubeID(0).batteryLevel()) / float(kMaxBatteryLevel); // TODO: Find out which CubeId we are dealing with in the menu
    batteryLevelBuddy = 0.4f; // XXX: test code
    drawBattery(icon, batteryLevelBuddy, levelCounter, vec(1, 2));
    drawText(icon, "Buddy", vec(5, 2));
    
    float batteryLevelMaster = float(CubeID(0).batteryLevel()) / float(kMaxBatteryLevel); // TODO: use API for master battery
    batteryLevelMaster = 0.9f; // XXX: test code
    drawBattery(icon, batteryLevelMaster, levelCounter, vec(1, 5));
    drawText(icon, "Master", vec(5, 5));

    String<8> bufferCubes;
    bufferCubes << getNumCubes(cubes) << " cubes";
    drawText(icon, bufferCubes.c_str(), vec(3, 8));
    
    String<16> bufferBlocks;
    bufferBlocks << getFreeBlocks() << " blocks";
    drawText(icon, bufferBlocks.c_str(), vec(1, 10));
    
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
        if (levelCounter < BatteryBars.numFrames())
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
    
    Events::cubeFound.set(&StatusApplet::onCubeFound, &instance);
    Events::cubeLost.set(&StatusApplet::onCubeLost, &instance);

    menu.append(&instance);
}

void StatusApplet::onCubeFound(unsigned cubeId)
{
    ASSERT(!cubes.test(cubeId));
    cubes.mark(cubeId);
}

void StatusApplet::onCubeLost(unsigned cubeId)
{
    ASSERT(cubes.test(cubeId));
    cubes.clear(cubeId);
}
