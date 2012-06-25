/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(BootstrapGroup);

static AssetSlot AnimationSlot = AssetSlot::allocate();

static const unsigned numCubes = 3;
static const CubeSet allCubes(0, numCubes);
static VideoBuffer vid[numCubes];
static Metadata M = Metadata()
    .title("Asset Slot Example")
    .cubeRange(numCubes);


void main()
{
    ScopedAssetLoader loader;

    for (CubeID cube : allCubes) {
        vid[cube].initMode(BG0);
        vid[cube].bg0.image(vec(0,0), LoadingBg);
        vid[cube].attach(cube);
    }

    loader.start(LoadingGroup, MainSlot, allCubes);
    loader.finish();

    for (CubeID cube = 0; cube != numCubes; ++cube) {
        vid[cube].bg0.image(vec(0,5), LoadingText);
    }

    while (1)
        System::paint();
}
