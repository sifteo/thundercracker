/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "mainmenu.h"
#include <sifteo.h>
#include <sifteo/menu.h>
using namespace Sifteo;


void MainMenu::init()
{
    // XXX: Fake cubeset initialization, until we have real cube connect/disconnect
    cubes = CubeSet(3);
    _SYS_enableCubes(cubes);
}

void MainMenu::append(MainMenuItem *item)
{
}

void MainMenu::run()
{
    while (1)
        System::paint();
}
