/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

static Cube cube(0);

void load()
{
    cube.enable();
    cube.loadAssets(GameAssets);

    VidMode_BG0_ROM vid(cube.vbuf);
    vid.init();
    do {
        vid.BG0_progressBar(Vec2(0,7), cube.assetProgress(GameAssets, VidMode_BG0::LCD_width) & ~3, 2); 
        System::paint();
    } while (!cube.assetDone(GameAssets));
}

unsigned unsigned_mod(int x, unsigned y)
{
    // Modulo operator that always returns an unsigned result

    int z = x % (int)y;
    if (z < 0) z += y;
    return z;
}

void draw_bg_column(int x)
{
    // Draw a vertical column of tiles.
    // XXX: This should be folded into a rectangle blit syscall

    LOG(("Drawing column %d\n", x));

    uint16_t addr = unsigned_mod(x, 18);
    const uint16_t *src = Background.tiles + unsigned_mod(x, Background.width);
    
    for (unsigned i = 0; i < Background.height; i++) {
        _SYS_vbuf_writei(&cube.vbuf.sys, addr, src, 0, 1);
        addr += 18;
        src += Background.width;
    }
}

void main()
{
    load();
    
    VidMode_BG0 vid(cube.vbuf);
    vid.init();

    for (int x = -1; x < 17; x++)
        draw_bg_column(x);
                   
    float x = 0;
    int prev_xt = 0;
    
    while (1) {
        // Scroll based on accelerometer tilt
        Int2 acc = cube.physicalAccel();

        // Floating point pixels
        x += acc.x * (40.0f / 128.0f);
        
        // Integer pixels
        int xi = x + 0.5f;
        
        // Integer tiles
        int xt = x / 8;
        
        LOG(("Main loop: x=%f prev_xt=%d xt=%d\n", x, prev_xt, xt));
        
        while (prev_xt < xt) {
            // Fill in new tiles, just past the right edge of the screen
            draw_bg_column(prev_xt + 17);
            prev_xt++;
        }
        
        while (prev_xt > xt) {
            // Fill in new tiles, just past the left edge
            draw_bg_column(prev_xt - 2);
            prev_xt--;
        }
        
        // Firmware handles all pixel-level scrolling
        vid.BG0_setPanning(Vec2(xi, 0));
        
        System::paint();
    }
}
