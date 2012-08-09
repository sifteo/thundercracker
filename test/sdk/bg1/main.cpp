#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static AssetSlot MainSlot = AssetSlot::allocate();

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
    // Bootstrapping that would normally be done by the Launcher
    while (!CubeSet::connected().test(cube))
        System::yield();
    _SYS_asset_bindSlots(_SYS_fs_runningVolume(), 1);

    AssetConfiguration<1> config;
    ScopedAssetLoader loader;
    SCRIPT(LUA, System():setAssetLoaderBypass(true));
    config.append(MainSlot, MainAssets);
    loader.start(config, cube);
    loader.finish();

    SCRIPT(LUA,
        package.path = package.path .. ";../../lib/?.lua"
        require('siftulator')
        cube = Cube(0)
    );

    vid.initMode(BG0_SPR_BG1);
    vid.bg0.erase(Background);
    vid.attach(cube);

    testMaskedImage();

    LOG("Success.\n");
}