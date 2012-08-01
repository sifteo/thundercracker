/*
 * Sifteo SDK Example.
 */

#include "game.h"

AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(BootAssets);

static Metadata M = Metadata()
    .title("Membrane")
    .package("com.sifteo.sdk.membrane", "1.0")
    .icon(Icon)
    .cubeRange(NUM_CUBES);

void main()
{
    static Game game;

    while (1) {
	    game.title();
	    game.init();
	    game.run();
	    game.cleanup();
	}
}
