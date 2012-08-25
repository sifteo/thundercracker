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
    virtual void arrive();
    virtual void onCubeBatteryLevelChange(unsigned cid);
    virtual void onCubeConnect(unsigned cid);
    virtual void onCubeDisconnect(unsigned cid);
    virtual void onVolumeChanged(unsigned volumeHandle);

    
    static void add(Sifteo::Array<MainMenuItem*, Shared::MAX_ITEMS> &items);

private:
    void drawIcon(Sifteo::CubeID menuCube);
    void drawCube(Sifteo::CubeID cube);
    
    IconBuffer menuIcon;
};
