/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#include <sifteo.h>
#include <sifteo/menu.h>

class MainMenuItem;


/**
 * The Main Menu is the menu which can run applications in the Launcher.
 *
 * It differs from other menus in that it supports asset bootstrapping,
 * via the MainMenuItem abstract base class.
 */

class MainMenu
{
public:

    /**
     * Set up the Main Menu's data structures, and empty its lists of items.
     */
    void init();

    /**
     * Append a new item to the internal list of menu items. The MainMenuItem
     * object must remain valid until the menu is run.
     */
    void append(MainMenuItem *item);

    /**
     * Execute the menu's main loop. When an item is chosen, it is exec()'ed.
     * If exec() returns, this function will return.
     */
    void run();

private:
    Sifteo::CubeSet cubes;
};
