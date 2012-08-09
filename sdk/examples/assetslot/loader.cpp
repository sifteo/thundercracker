/*
 * Sifteo SDK Example.
 */

#include "loader.h"
#include "assets.gen.h"
using namespace Sifteo;


MyLoader::MyLoader(CubeSet cubes, AssetSlot loaderSlot, VideoBuffer *vid)
    : cubes(cubes), loaderSlot(loaderSlot), vid(vid)
{
    assetLoader.init();
}


void MyLoader::load(AssetGroup &group, AssetSlot slot)
{
    LOG("Loader, (%P, %d): starting\n", &group, slot.sys);

    // The bootstrap group should already be installed on all cubes.
    ASSERT(BootstrapGroup.isInstalled(cubes));

    // Early out if the group in question is already loaded
    if (group.isInstalled(cubes)) {
        LOG("Loader, (%P, %d): already installed at (%x, %x, %x)\n",
            &group, slot.sys,
            group.baseAddress(0),
            group.baseAddress(1),
            group.baseAddress(2));
        return;
    }

    // Draw a background image from the bootstrap asset group.
    for (CubeID cube : cubes) {
        vid[cube].initMode(BG0);
        vid[cube].attach(cube);

        /*
         * Must draw to the buffer after attaching it to a cube, so we
         * know how to relocate this image's tiles for the cube in question.
         */
        vid[cube].bg0.image(vec(0,0), LoadingBg);
    }

    // Draw the background
    System::paint();
    System::finish();

    AssetConfiguration<1> config;
    config.append(slot, group);

    // Immediately start asynchronously loading the group our caller requested
    assetLoader.start(config, cubes);

    /*
     * Animate the loading process, using STAMP mode to draw an animated progress bar.
     */

    for (CubeID cube : cubes) {
        vid[cube].initMode(STAMP);
        vid[cube].setWindow(100, 10);
        vid[cube].stamp.disableKey();

        auto& fb = vid[cube].stamp.initFB<16, 16>();

        for (unsigned y = 0; y < 16; ++y)
            for (unsigned x = 0; x < 16; ++x)
                fb.plot(vec(x,y), (-x-y) & 0xF);
    }

    unsigned frame = 0;
    while (!assetLoader.isComplete()) {

        for (CubeID cube : cubes) {

            // Animate the horizontal window, to show progress
            unsigned p = 1 + assetLoader.cubeProgress(cube, LCD_width - 1);
            vid[cube].stamp.setHWindow(0, p);

            LOG("Progress on cube %d: %d / %d (%d)\n", int(cube),
                assetLoader.cubes[cube].progress,
                assetLoader.cubes[cube].total,
                p);

            // Animate the colormap at a steady rate
            const RGB565 bg = RGB565::fromRGB(0xff7000);
            const RGB565 fg = RGB565::fromRGB(0xffffff);

            auto &cmap = vid[cube].colormap;
            cmap.fill(bg);
            for (unsigned i = 0; i < 8; i++)
                cmap[(frame + i) & 15] = bg.lerp(fg, i << 5);
        }

        // Throttle our frame rate, to load assets faster
        for (unsigned i = 0; i != 4; ++i)
            System::paint();

        frame++;
    }
    
    LOG("Loader, (%P, %d): done\n", &group, slot.sys);
}
