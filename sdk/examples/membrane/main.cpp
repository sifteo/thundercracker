/* Copyright <c> 2011 Sifteo, Inc. All rights reserved. */

#include "game.h"

void siftmain()
{
    static Game game;

    game.loadAssets();
    game.title();
    game.init();
    game.run();
}
