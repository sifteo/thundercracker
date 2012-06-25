/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static const unsigned gNumCubes = 1;
static VideoBuffer vid[CUBE_ALLOCATION];

static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(BootstrapGroup);

static AssetSlot AnimationSlot = AssetSlot::allocate();

static Metadata M = Metadata()
    .title("Asset Slot Example")
    .cubeRange(gNumCubes);


void main()
{
}
