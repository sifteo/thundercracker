/* Copyright <c> 2011 Sifteo, Inc. All rights reserved. */

#include "game.h"

AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(BootAssets);

static Metadata M = Metadata()
    .title("Membrane")
    .cubeRange(NUM_CUBES);

void main()
{
    static Game game;

    game.title();
    game.init();
    game.run();
}
