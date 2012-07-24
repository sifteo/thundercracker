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
    virtual MainMenuItem::Flags getAssets(Sifteo::MenuItem &assets, Sifteo::MappedVolume &);
    virtual bool autoRefreshIcon() { return true; }

    virtual void exec();
    virtual void arrive();
    virtual void depart();
    virtual void prepaint();

    static void add(MainMenu &menu);

private:
    Sifteo::RelocatableTileBuffer<12,12> icon;
    unsigned levelCounter;
};
