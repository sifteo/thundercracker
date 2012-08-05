/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#include <sifteo.h>
#include "mainmenuitem.h"

class MainMenu;

class StatusApplet : public MainMenuItem
{
public:
    virtual void getAssets(Sifteo::MenuItem &assets, Shared::AssetConfiguration &config);
    
    virtual void exec();
    virtual void arrive(Sifteo::Menu &m, unsigned index);
    virtual void depart(Sifteo::Menu &m, unsigned index);
    
    static void add(MainMenu &m);

private:
    void drawIcon();
    void drawCube(Sifteo::CubeID cube);
    
    void onBatteryLevelChange(unsigned cid);
    
    Sifteo::Menu *menu;
    int menuItemIndex;
    Sifteo::RelocatableTileBuffer<12,12> menuIcon;
};
