/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(GameAssets);

static Metadata M = Metadata()
    .title("BG1 Scroller SDK Example")
    .package("com.sifteo.sdk.scroller-bg1", "1.0")
    .cubeRange(1);

void drawBG0Column(VideoBuffer &vid, int x)
{
    // Draw a vertical column of tiles on BG0

    int bg0Width = vid.bg0.tileWidth();
    int dstx = umod(x, bg0Width);
    int srcx = umod(x, Background.tileWidth());

    vid.bg0.image(vec(dstx, 0), vec(1, bg0Width), Background, vec(srcx, 0));
}

void main()
{
    CubeID cube(0);

    VideoBuffer vid;
    vid.initMode(BG0_BG1);
    vid.attach(cube);

    // create a BG1 mask, initially allocating tiles for the Guage along the top of the screen
    BG1Mask bg1mask = BG1Mask::filled(vec(0,0), vec(Gauge.tileWidth(), Gauge.tileHeight()));

    // Allocate tiles for the Sprite as well
    bg1mask.fill(vec(2, 4), vec(Sprite.tileWidth(), Sprite.tileHeight()));

    // apply the tile mask, and draw the Gauge
    vid.bg1.setMask(bg1mask);
    vid.bg1.image(vec(0, 0), Gauge);

    // draw initial BG0 layer
    for (unsigned x = 0; x < 17; x++) {
        drawBG0Column(vid, x);
    }

    /*
     * Walk the Sprite in BG1 along the scrolling BG0 layer.
     *
     * Pre-load new columns of BG0 as we approach the edge of the currently loaded tiles.
     * Cycle through the Sprite animation in place on BG1.
     */

    unsigned frame = 0;

    for (;;) {
        unsigned pan_x = frame * 3;

        // Fill in new tiles, just past the right edge of the screen
        drawBG0Column(vid, pan_x / 8 + 17);

        // Animate our sprite on the BG1 layer
        vid.bg1.image(vec(2, 4), Sprite, umod(frame, Sprite.numFrames()));

        vid.bg0.setPanning(vec(pan_x, 0u));

        System::paint();
        frame++;
    }
}