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
static void drawBattery(T &canvas, float batteryLevel, Int2 pos)
{
    unsigned numBatteryLevels = ceil(batteryLevel * float(BatteryBars.numFrames()));
    
    if (numBatteryLevels > 0) {
        --numBatteryLevels;
        ASSERT(numBatteryLevels < BatteryBars.numFrames());
        canvas.image(pos, BatteryBars, numBatteryLevels);
    } else {
        canvas.image(pos, BatteryBars, 0);
    }
}

static void drawText(MainMenuItem::IconBuffer &icon, const char* text, Int2 pos)
{
    for (int i = 0; text[i] != 0; i++) {
        icon.image(vec(pos.x + i, pos.y), Font, text[i]-32);
    }
}

static unsigned getFreeMemory()
{
    FilesystemInfo info;
    info.gather();
    
    float percentage = float(info.freeUnits()) / float(info.totalUnits());
    
    return unsigned(percentage * 100);
}

void StatusApplet::getAssets(Sifteo::MenuItem &assets, Shared::AssetConfiguration &config)
{
    drawIcon(CubeID());
    assets.icon = menuIcon;
}

void StatusApplet::exec()
{
}

void StatusApplet::arrive(Sifteo::Menu &m, unsigned index)
{
    menu = &m;
    menuItemIndex = index;
    
    // Update menu icon, if necessary
    if (numCubes != CubeSet::connected().count()) {
        drawIcon(m.cube());
        menu->replaceIcon(menuItemIndex, menuIcon);
    }

    // Draw Icon Background
    for (CubeID cube : CubeSet::connected()) {
        if (cube != m.cube()) {
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
        if (cube != m.cube()) {
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

void StatusApplet::drawIcon(Sifteo::CubeID menuCube)
{
    menuIcon.init();
    menuIcon.image(vec(0,0), Icon_Status);

    if (menuCube.isDefined()) {
        drawBattery(menuIcon, menuCube.batteryLevel(), vec(8, 1));
    }
    drawBattery(menuIcon, System::batteryLevel(), vec(7, 7));
    
    numCubes = CubeSet::connected().count();
    
    String<8> bufferCubes;
    bufferCubes << numCubes;
    drawText(menuIcon, bufferCubes.c_str(), vec(numCubes < 10 ? 4 : 3, 4));
    
    String<16> bufferBlocks;
    bufferBlocks << getFreeMemory();
    drawText(menuIcon, bufferBlocks.c_str(), vec(1, 10));
}

void StatusApplet::drawCube(CubeID cube)
{
    auto &vid = Shared::video[cube];
    vid.initMode(BG0);
    vid.bg0.erase(Menu_StripeTile);
    vid.bg0.image(vec(2,2), Icon_StatusOther);
    drawBattery(vid.bg0, cube.batteryLevel(), vec(7, 9));
}

void StatusApplet::onBatteryLevelChange(unsigned cid)
{
    if (menu != NULL && menuItemIndex >= 0) {
        CubeID menuCube = menu->cube();

        if (cid == menuCube) {
            drawIcon(menuCube);
            menu->replaceIcon(menuItemIndex, menuIcon);
        } else {
            drawCube(CubeID(cid));
        }
    }
}
