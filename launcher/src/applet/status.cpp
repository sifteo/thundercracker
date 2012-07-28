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

static const float kBatteryLevelLow = 0.25f;

template<typename T>
static void drawBattery(T &canvas, float batteryLevel, Int2 pos, const AssetImage &battery)
{
    canvas.image(vec(pos.x-1, pos.y-1), battery);

    unsigned numBatteryLevels = ceil(batteryLevel * float(BatteryBars.numFrames()));
    
    if (numBatteryLevels > 0)
    {
        --numBatteryLevels;
        ASSERT(numBatteryLevels < BatteryBars.numFrames());
        canvas.image(pos, BatteryBars, numBatteryLevels);
    }
}

static void drawText(RelocatableTileBuffer<12,12> &icon, const char* text, Int2 pos)
{
    for (int i = 0; text[i] != 0; i++) {
        icon.image(vec(pos.x + i, pos.y), Font, text[i]-32);
    }
}

static unsigned getNumCubes(CubeSet cubes)
{
    unsigned count = 0;
    for (CubeID cube : cubes) {
        ++count;
    }
    return count;
}

static unsigned getFreeBlocks()
{
    // TODO: amount of free blocks
    return 128;
}

CubeID getMainCube()
{
    return *CubeSet::connected().begin();
}

MainMenuItem::Flags StatusApplet::getAssets(MenuItem &assets, MappedVolume &)
{
    icon.init();
    icon.image(vec(0,0), Icon_Battery);

    drawBattery(icon, getMainCube().batteryLevel(), vec(1, 2), BatteryCube);
    drawText(icon, "Cube", vec(5, 2));
    
    drawBattery(icon, System::batteryLevel(), vec(1, 5), BatteryBase);
    drawText(icon, "Base", vec(5, 5));

    String<8> bufferCubes;
    bufferCubes << getNumCubes(CubeSet::connected()) << "x";
    drawText(icon, bufferCubes.c_str(), vec(2, 8));
    
    icon.image(vec(5, 6), Cube);
    
    String<16> bufferBlocks;
    bufferBlocks << getFreeBlocks() << " blocks";
    drawText(icon, bufferBlocks.c_str(), vec(1, 10));
    
    assets.icon = icon;
    return NONE;
}

void StatusApplet::exec()
{
}

void StatusApplet::arrive()
{
    // Draw Icon Background
    for (CubeID cube : CubeSet::connected()) {
        if (cube != getMainCube()) {
            auto &vid = Shared::video[cube];
            vid.bg0.image(vec(2, 2), Icon_Battery);
        }
    }

    // Low Battery SFX
    if (System::batteryLevel() <= kBatteryLevelLow) {
        AudioChannel(0).play(Sound_BatteryLowBase);
    } else {
        for (CubeID cube : CubeSet::connected()) {
            if (cube.batteryLevel() <= kBatteryLevelLow) {
                AudioChannel(0).play(Sound_BatteryLowCube);
                break;
            }
        }
    }
}

void StatusApplet::depart()
{
    // Display a background on all other cubes
    for (CubeID cube : CubeSet::connected()) {
        if (cube != getMainCube()) {
            auto &vid = Shared::video[cube];
            vid.initMode(BG0);
            vid.attach(cube);
            vid.bg0.erase(Menu_StripeTile);
        }
    }
}

void StatusApplet::prepaint()
{
    for (CubeID cube : CubeSet::connected()) {
        if (cube != getMainCube()) {
            auto &vid = Shared::video[cube];
            vid.bg0.image(vec(2,2), Icon_Battery);
            drawBattery(vid.bg0, cube.batteryLevel(), vec(4, 4), BatteryCube);
        }
    }
}

void StatusApplet::add(MainMenu &menu)
{
    static StatusApplet instance;
    menu.append(&instance);
}
