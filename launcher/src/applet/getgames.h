/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#include <sifteo.h>
#include "mainmenuitem.h"

class MainMenu;


class GetGamesApplet : public MainMenuItem
{
public:
    virtual void getAssets(Sifteo::MenuItem &assets);
    virtual void exec();

    static void add(MainMenu &menu);
};
