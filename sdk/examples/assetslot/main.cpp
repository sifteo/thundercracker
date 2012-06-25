/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include "assets.gen.h"
#include "loader.h"
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


void spinnyAnimation()
{
    MyLoader loader(allCubes, MainSlot, vid);

    loader.load(SpinnyGroup, AnimationSlot);

    for (CubeID cube : allCubes)
        vid[cube].initMode(BG0);

    while (1) {
        unsigned frame = SystemTime::now().cycleFrame(2.0, Spinny.numFrames());
        for (CubeID cube : allCubes)
            vid[cube].bg0.image(vec(0,0), Spinny, frame);
        System::paint();
    }
}


void main()
{
    spinnyAnimation();
}
