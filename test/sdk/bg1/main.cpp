#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(MainAssets);

static Metadata M = Metadata()
    .title("BG1 test")
    .cubeRange(1);

static VideoBuffer vid;
const CubeID cube(0);

void testMaskedImage()
{
    for (unsigned f = 0; f < Animation.numFrames(); ++f) {
        vid.bg1.maskedImage(Animation, Transparent, f);
        vid.bg0.setPanning(vid.bg0.getPanning() + vec(1,0));

        System::paint();
        System::finish();

        SCRIPT_FMT(LUA, "util:assertScreenshot(cube, 'maskedImage-%02d')", f);
    }
}

void main()
{
    SCRIPT(LUA,
        package.path = package.path .. ";../../lib/?.lua"
        require('siftulator')
        cube = Cube(0)
    );

    vid.initMode(BG0_SPR_BG1);
    vid.bg0.erase(Background);
    vid.attach(cube);

    testMaskedImage();
}