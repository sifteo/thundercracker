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

void siftmain()
{
    load();
    
    VidMode_BG0 vid(cube.vbuf);
    vid.init();
    vid.BG0_drawAsset(Vec2(0,0), Background);
 
    _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_BG1_BITMAP/2, 0, 16);
    _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_SPR, 0, 8*5/2);
    
    for (unsigned i = 0; i < _SYS_VRAM_SPRITES; i++) {
        _SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, spr[0].mask_x)
                        + sizeof(_SYSSpriteInfo) * i, -16);
        _SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, spr[0].mask_y)
                        + sizeof(_SYSSpriteInfo) * i, -16);
    }
    
    _SYS_vbuf_fill(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2
                   + 16 - Overlay.height,
                   ((1 << Overlay.width) - 1), Overlay.height);
    _SYS_vbuf_writei(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2,
                     Overlay.tiles, 0, Overlay.width * Overlay.height);
        
    _SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);

    unsigned frame = 0;
    while (1) {
        frame++;

        for (unsigned i = 0; i < _SYS_VRAM_SPRITES; i++) {
            
            float angle = frame * 0.075f + i * (M_PI*2/_SYS_VRAM_SPRITES);
            const float r = 32;
            int8_t x = (128 - 16)/2 + cosf(angle) * r + 0.5f;
            int8_t y = (128 - 16)/2 + sinf(angle) * r + 0.5f;
    
            _SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, spr[0].pos_x)
                        + sizeof(_SYSSpriteInfo) * i, -x);
            _SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, spr[0].pos_y)
                        + sizeof(_SYSSpriteInfo) * i, -y);
                        
            _SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_x), -frame);
        }
    
        System::paint();
    }
}
