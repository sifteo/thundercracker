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
    virtual MainMenuItem::Flags getAssets(Sifteo::MenuItem &assets, Sifteo::MappedVolume&);
    virtual bool autoRefreshIcon() { return true; }

    virtual void exec();
    virtual void arrive(Sifteo::CubeSet cubes, Sifteo::CubeID mainCube);
    virtual void depart(Sifteo::CubeSet cubes, Sifteo::CubeID mainCube);
    virtual void prepaint(Sifteo::CubeSet cubes, Sifteo::CubeID mainCube);

    static void add(MainMenu &menu);

private:
    Sifteo::RelocatableTileBuffer<12,12> icon;
    unsigned levelCounter;
};
