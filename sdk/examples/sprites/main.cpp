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

void moveSprite(int id, int x, int y)
{
    uint8_t xb = -x;
    uint8_t yb = -y;

    uint16_t word = ((uint16_t)xb << 8) | yb;
    uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].pos_y)/2 +
                      sizeof(_SYSSpriteInfo)/2 * id ); 

    _SYS_vbuf_poke(&cube.vbuf.sys, addr, word);
}

void resizeSprite(int id, int w, int h)
{
    uint8_t xb = -w;
    uint8_t yb = -h;

    uint16_t word = ((uint16_t)xb << 8) | yb;
    uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].mask_y)/2 +
                      sizeof(_SYSSpriteInfo)/2 * id ); 

    _SYS_vbuf_poke(&cube.vbuf.sys, addr, word);
}

void setSpriteImage(int id, int tile)
{
    uint16_t word = VideoBuffer::indexWord(tile);
    uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].tile)/2 +
                      sizeof(_SYSSpriteInfo)/2 * id ); 

    _SYS_vbuf_poke(&cube.vbuf.sys, addr, word);
}
 

void siftmain()
{
    load();
    
    VidMode_BG0 vid(cube.vbuf);
    vid.init();
    vid.BG0_drawAsset(Vec2(0,0), Background);
 
    // Init BG1/Sprite VRAM
    _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_BG1_BITMAP/2, 0, 16);
    _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_SPR, 0, 8*5/2);
    
    // 1UPs
    for (unsigned i = 1; i < _SYS_VRAM_SPRITES; i++) {
        resizeSprite(i, Sprite.width*8, Sprite.height*8);
        setSpriteImage(i, Sprite.index);
    }

    // Bullet
    resizeSprite(0, Bullet.width*8, Bullet.height*8);
    setSpriteImage(0, Bullet.index);

    // BG1 Overlay
    _SYS_vbuf_fill(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2
                   + 16 - Overlay.height,
                   ((1 << Overlay.width) - 1), Overlay.height);
    _SYS_vbuf_writei(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2,
                     Overlay.tiles, 0, Overlay.width * Overlay.height);
        
    // Now enable BG0, Sprites, and BG1
    _SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);

    unsigned frame = 0;
    while (1) {
        frame++;

        // Circle of 1UPs
        for (unsigned i = 1; i < _SYS_VRAM_SPRITES; i++) {
            
            float angle = frame * 0.075f + (i-1) * (M_PI*2 / (_SYS_VRAM_SPRITES-1));
            const float r = 32;

            moveSprite(i, (128 - 16)/2 + cosf(angle) * r + 0.5f,
                       (128 - 16)/2 + sinf(angle) * r + 0.5f); 
        }

        // Scroll BG1
        _SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_x), -frame);
    
        // Flying bullet
        moveSprite(0, 130-frame*3, 190-frame);

        System::paint();
    }
}
