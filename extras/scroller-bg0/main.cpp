/*
 * Sifteo SDK Example.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(GameAssets);

static Metadata M = Metadata()
    .title("BG0 Scroller SDK Example")
    .package("com.sifteo.sdk.scroller-bg0", "1.0")
    .cubeRange(1);

void drawColumn(VideoBuffer &vid, int x)
{
    // Draw a vertical column of tiles

    int bg0Width = vid.bg0.tileWidth();
    int dstx = umod(x, bg0Width);
    int srcx = umod(x, Background.tileWidth());

    vid.bg0.image(vec(dstx, 0), vec(1, bg0Width), Background, vec(srcx, 0));
}

void main()
{
    CubeID cube(0);

    VideoBuffer vid;
    vid.initMode(BG0);
    vid.attach(cube);

    for (int x = -1; x < 17; x++) {
        drawColumn(vid, x);
    }

   /*
    * Scroll horizontally through our background based on the accelerometer's X axis.
    *
    * We can scroll with pixel accuracy within a column of tiles via bg0.setPanning().
    * When we get to either edge of the currently plotted tiles, draw a new column
    * of tiles from the source image.
    * 
    * Because BG0 is 1 tile wider and taller than the viewport itself, we can pre-load
    * the next column of tiles into the column at the edge before it comes into view.
    */

    float x = 0;
    int prev_xt = 0;

    for (;;) {
        // Scroll based on accelerometer tilt
        Int2 accel = vid.physicalAccel().xy();

        // Floating point pixels
        x += accel.x * (40.0f / 128.0f);

        // Integer pixels
        int xi = x + 0.5f;

        // Integer tiles
        int xt = x / 8;

        while (prev_xt < xt) {
            // Fill in new tiles, just past the right edge of the screen
            drawColumn(vid, prev_xt + 17);
            prev_xt++;
        }

        while (prev_xt > xt) {
            // Fill in new tiles, just past the left edge
            drawColumn(vid, prev_xt - 2);
            prev_xt--;
        }

        // pixel-level scrolling within the current column
        vid.bg0.setPanning(vec(xi, 0));

        System::paint();
    }
}
