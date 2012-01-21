/* Copyright <c> 2011 Sifteo, Inc. All rights reserved. */

#include <stdlib.h>
#include <sifteo.h>

#include "game.h"


void Sprites::init()
{
    _SYS_vbuf_fill(&vbuf.sys, _SYS_VA_BG1_BITMAP/2, 0, 16);
    _SYS_vbuf_fill(&vbuf.sys, _SYS_VA_SPR, 0, 8*5/2);
    _SYS_vbuf_pokeb(&vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);
}

void Sprites::move(int id, Vec2 pos)
{
    uint8_t xb = -pos.x;
    uint8_t yb = -pos.y;

    uint16_t word = ((uint16_t)xb << 8) | yb;
    uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].pos_y)/2 +
                      sizeof(_SYSSpriteInfo)/2 * id ); 

    _SYS_vbuf_poke(&vbuf.sys, addr, word);
}

void Sprites::hide(int id)
{
    resizeSprite(id, 0, 0);
}

void Sprites::set(int id, const PinnedAssetImage &image, int frame)
{
    resizeSprite(id, image.width * 8, image.height * 8);
    setSpriteImage(id, image.index + (image.width * image.height) * frame);
}

void Sprites::resizeSprite(int id, int w, int h)
{
    uint8_t xb = -w;
    uint8_t yb = -h;

    uint16_t word = ((uint16_t)xb << 8) | yb;
    uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].mask_y)/2 +
                      sizeof(_SYSSpriteInfo)/2 * id ); 

    _SYS_vbuf_poke(&vbuf.sys, addr, word);
}

void Sprites::setSpriteImage(int id, int tile)
{
    uint16_t word = VideoBuffer::indexWord(tile);
    uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].tile)/2 +
                      sizeof(_SYSSpriteInfo)/2 * id ); 

    _SYS_vbuf_poke(&vbuf.sys, addr, word);
}

