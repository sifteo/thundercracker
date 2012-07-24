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
    unsigned numBatteryLevels = ceil(batteryLevel * float(BatteryBars.numFrames()));
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

float getBatteryLevelMaster() {
    //return float(CubeID(0).batteryLevel()) / float(kMaxBatteryLevel);
    return 0.9f; // XXX: replace with real master battery API
}

float getBatteryLevelCube(CubeID cube) {
    // XXX: put this in once batteryLevel() returns the real values
    //return float(cube.batteryLevel()) / float(kMaxBatteryLevel);
    switch (cube) {
        default:
        case 0: return 0.1f;
        case 1: return 0.5f;
        case 2: return 1.0f;
        case 3: return 1.0f;
    }
}

MainMenuItem::Flags StatusApplet::getAssets(MenuItem &assets, MappedVolume &)
{
    icon.init();
    icon.image(vec(0,0), Icon_Battery);

    drawBattery(icon, getBatteryLevelCube(CubeID(0)), levelCounter, vec(1, 2));
    drawText(icon, "Buddy", vec(5, 2));
    
    drawBattery(icon, getBatteryLevelMaster(), levelCounter, vec(1, 5));
    drawText(icon, "Master", vec(5, 5));

    String<8> bufferCubes;
    bufferCubes << getNumCubes(CubeSet::connected()) << " cubes";
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

void StatusApplet::arrive(CubeID mainCube)
{
    levelCounter = 0;

    for (CubeID cube : CubeSet::connected()) {
        if (cube != mainCube) {
            auto &vid = Shared::video[cube];
            vid.bg0.image(vec(2, 2), Icon_Battery);
        }
    }

    // Low battery SFX
    if (getBatteryLevelMaster() <= 0.25f) {
        AudioChannel(0).play(Sound_BatteryLowBase);
    } else {
        for (CubeID cube : CubeSet::connected()) {
            if (getBatteryLevelCube(cube) <= 0.25f) {
                AudioChannel(0).play(Sound_BatteryLowCube);
                break;
            }
        }
    }
}

void StatusApplet::depart(CubeID mainCube)
{
    // Display a background on all other cubes
    for (CubeID cube : CubeSet::connected())
        if (cube != mainCube) {
            auto &vid = Shared::video[cube];
            vid.initMode(BG0);
            vid.attach(cube);
            vid.bg0.erase(Menu_StripeTile);
        }
}

void StatusApplet::prepaint(CubeID mainCube)
{
    static bool frame = true;
    if (frame) {
        if (levelCounter < BatteryBars.numFrames())
        {
            ++levelCounter;
        }
    }
    frame = !frame;

    for (CubeID cube : CubeSet::connected()) {
        if (cube != mainCube) {
            auto &vid = Shared::video[cube];
            vid.bg0.image(vec(2,2), Icon_Battery);
            drawBattery(vid.bg0, getBatteryLevelCube(cube), levelCounter, vec(4, 4));
        }
    }
}

void StatusApplet::add(MainMenu &menu)
{
    static StatusApplet instance;
    menu.append(&instance);
}
