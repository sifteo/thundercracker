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
    virtual void exec();
    virtual void arrive(Sifteo::CubeSet cubes);
    virtual void depart(Sifteo::CubeSet cubes);

    static void add(MainMenu &menu);
};
