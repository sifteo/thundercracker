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
    
    if (numBatteryLevels > 0) {
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

static unsigned getFreeMemory()
{
    FilesystemInfo info;
    info.gather();
    
    float percentage = float(info.freeUnits()) / float(info.totalUnits());
    
    return unsigned(percentage * 100);
}

CubeID getMainCube()
{
    return *CubeSet::connected().begin();
}

MainMenuItem::Flags StatusApplet::getAssets(MenuItem &assets, MappedVolume &)
{
    drawIcon();
    
    assets.icon = menuIcon;
    return NONE;
}

void StatusApplet::exec()
{
}

void StatusApplet::arrive(Sifteo::Menu &m, unsigned index)
{
    menu = &m;
    menuItemIndex = index;
    
    // Draw Icon Background
    for (CubeID cube : CubeSet::connected()) {
        if (cube != getMainCube()) {
            drawCube(cube);
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

void StatusApplet::depart(Sifteo::Menu &m, unsigned index)
{
    // Display a background on all other cubes
    for (CubeID cube : CubeSet::connected()) {
        if (cube != getMainCube()) {
            Shared::video[cube].bg0.erase(Menu_StripeTile);
        }
    }
    
    menu = NULL;
    menuItemIndex = -1;
}

void StatusApplet::add(MainMenu &m)
{
    static StatusApplet instance;
    instance.menu = NULL;
    instance.menuItemIndex = -1;
    Events::cubeBatteryLevelChange.set(&StatusApplet::onBatteryLevelChange, &instance);
    m.append(&instance);
}

void StatusApplet::drawIcon()
{
    menuIcon.init();
    menuIcon.image(vec(0,0), Icon_Battery);

    drawBattery(menuIcon, getMainCube().batteryLevel(), vec(1, 2), BatteryCube);
    drawText(menuIcon, "Cube", vec(5, 2));
    
    drawBattery(menuIcon, System::batteryLevel(), vec(1, 5), BatteryBase);
    drawText(menuIcon, "Base", vec(5, 5));

    String<8> bufferCubes;
    bufferCubes << getNumCubes(CubeSet::connected()) << "x";
    drawText(menuIcon, bufferCubes.c_str(), vec(2, 8));
    
    menuIcon.image(vec(5, 6), Cube);
    
    String<16> bufferBlocks;
    bufferBlocks << getFreeMemory() << "\% free";
    drawText(menuIcon, bufferBlocks.c_str(), vec(1, 10));
}

void StatusApplet::drawCube(CubeID cube)
{
    auto &vid = Shared::video[cube];
    vid.initMode(BG0);
    vid.bg0.erase(Menu_StripeTile);
    vid.bg0.image(vec(2,2), Icon_Battery);
    drawBattery(vid.bg0, cube.batteryLevel(), vec(4, 4), BatteryCube);
}

void StatusApplet::onBatteryLevelChange(unsigned cid)
{
    if (menu != NULL && menuItemIndex >= 0) {
        if (cid == getMainCube()) {
            drawIcon();
            menu->replaceIcon(menuItemIndex, menuIcon);
        } else {
            drawCube(CubeID(cid));
        }
    }
}
