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

void main()
{
    load();
    
    VidMode_BG0_SPR_BG1 vid(cube.vbuf);
    vid.init();
    vid.BG0_drawAsset(Vec2(0,0), Background);
     
    // 1UPs
    for (unsigned i = 1; i < _SYS_VRAM_SPRITES; i++) {
        vid.resizeSprite(i, Sprite.width*8, Sprite.height*8);
        vid.setSpriteImage(i, Sprite.index);
    }

    // Bullet
    vid.resizeSprite(0, Bullet.width*8, Bullet.height*8);
    vid.setSpriteImage(0, Bullet.index);

    // BG1 Overlay
    _SYS_vbuf_fill(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2
                   + 16 - Overlay.height,
                   ((1 << Overlay.width) - 1), Overlay.height);
    _SYS_vbuf_writei(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2,
                     Overlay.tiles, 0, Overlay.width * Overlay.height);

    unsigned frame = 0;
    while (1) {
        frame++;

        // Circle of 1UPs
        for (unsigned i = 1; i < _SYS_VRAM_SPRITES; i++) {
            
            float angle = frame * 0.075f + (i-1) * (M_PI*2 / (_SYS_VRAM_SPRITES-1));
            const float r = 32;
            const Float2 center = { (128 - 16)/2, (128 - 16)/2 };

            vid.moveSprite(i, Int2(center + polar(angle, r)).round());
        }

        // Scroll BG1
        vid.BG1_setPanning(Vec2(-frame, 0u));
        
        // Flying bullet
        vid.moveSprite(0, 130-frame*3, 190-frame);

        System::paint();
    }
}
