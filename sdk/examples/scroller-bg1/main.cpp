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

void draw_bg_column(unsigned x)
{
    uint16_t addr = (x % 18);
    const uint16_t *src = Background.tiles + (x % Background.width);
    
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

    // Enter BG0+BG1 mode
    _SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_BG1);
    
    // Allocate tiles for the sprite 
    _SYS_vbuf_fill(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2 + 4,
                   ((1 << Sprite.width) - 1) << 1, Sprite.height);

    // Allocate tiles for the static gauge, and draw it.
    _SYS_vbuf_fill(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2,
                   ((1 << Gauge.width) - 1), Gauge.height);
    _SYS_vbuf_writei(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2,
                     Gauge.tiles, 0, Gauge.width * Gauge.height);
                   
    for (unsigned x = 0; x < 17; x++)
        draw_bg_column(x);
                   
    unsigned frame = 0;
    while (1) {
        int pan_x = frame*3;
        
        // Fill in new tiles, just past the right edge of the screen
        draw_bg_column(pan_x/8 + 17);
    
        // Animate our sprite on the BG1 layer
        _SYS_vbuf_writei(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2 + Gauge.width * Gauge.height,
                         Sprite.tiles + (frame/2 % Sprite.frames) * (Sprite.width * Sprite.height),
                         0, Sprite.width * Sprite.height);

        vid.BG0_setPanning(Vec2(pan_x, 0));
        
        System::paint();
        frame++;
    }
}