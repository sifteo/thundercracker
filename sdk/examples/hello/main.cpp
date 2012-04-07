/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static const unsigned gNumCubes = 1;
static VideoBuffer vid[CUBE_ALLOCATION];

static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(BootAssets);

static Metadata M = Metadata()
    .title("Hello World SDK Example")
    .icon(GameIcon)
    .cubeRange(gNumCubes);


static void loadAssets()
{
    /*
     * Load our main assets, while animating Kirby walking across the screen
     */

    AssetLoader loader;
    loader.init();

    for (CubeID cube = 0; cube < gNumCubes; ++cube) {
        // Set up a blank white screen on this cube
        cube.enable();
        vid[cube].initMode(BG0);
        vid[cube].attach(cube);
        vid[cube].bg0.erase(WhiteTile);

        // Start asynchronously loading MainGroup
        loader.start(MainAssets, MainSlot, cube);
    } 

    int frame = 0;
    while (!loader.isComplete()) {

        // Animate Kirby running across the screen as MainAssets loads.
        for (CubeID cube = 0; cube < gNumCubes; ++cube) {
            auto &draw = vid[cube].bg0;
            const int xMin = -16;
            const int xMax = 16 + LCD_width - Kirby.pixelWidth();
            const int yMiddle = (LCD_height - Kirby.pixelHeight()) / 2;
            
            Int2 pan = vec(loader.progress(cube, xMin, xMax), yMiddle);
            LOG_INT2(pan);

            draw.image(vec(0,0), Kirby, frame);
            draw.setPanning(-pan);
        }

        if (++frame == Kirby.numFrames())
            frame = 0;

        /*
         * Frame rate limit: Drawing has higher priority than asset loading,
         * so in order to load assets quickly we need to explicitly leave some
         * time for the system to send asset data over the radio.
         */
        for (unsigned i = 0; i < 4; i++)
            System::paint();
    }

    loader.finish();
}

static void onAccelChange(void *, unsigned cid)
{
    /*
     * Event callback for Accelerometer input. Draw it to the screen.
     */
     
    auto accel = CubeID(cid).accel();
    auto &draw = vid[cid].bg0;

    String<64> str;
    str << "Accel: " << Hex(accel.x + 0x80, 2) << " " << Hex(accel.y + 0x80, 2);
    LOG_STR(str);

    draw.text(vec(2,3), Font, str);
    draw.setPanning(accel.xy() / -2);
}

static void helloWorld()
{
    /*
     * Hello World! Do some animation and react to accelerometer data.
     */

    Events::cubeAccelChange.set(onAccelChange);

    for (CubeID cube = 0; cube < gNumCubes; ++cube) {
        auto &draw = vid[cube].bg0;

        draw.erase(WhiteTile);
        draw.text(vec(2,1), Font, "Hello World!");
        draw.image(vec(1,10), Logo);

        onAccelChange(0, cube);
    } 
    
    int frame = 0;
    while (1) {
        for (CubeID cube = 0; cube < gNumCubes; ++cube) {
            auto &draw = vid[cube].bg0;
            draw.image(vec(7,6), Ball, frame);
        }
        if (++frame == Ball.numFrames())
            frame = 0;

        System::paint();
    }
}

void main()
{
    loadAssets();
    helloWorld();
}