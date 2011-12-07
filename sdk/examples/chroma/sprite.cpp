/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "sprite.h"

void moveSprite(Cube &cube, int id, int x, int y)
{
    uint8_t xb = -x;
    uint8_t yb = -y;

    uint16_t word = ((uint16_t)xb << 8) | yb;
    uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].pos_y)/2 +
                      sizeof(_SYSSpriteInfo)/2 * id ); 

    _SYS_vbuf_poke(&cube.vbuf.sys, addr, word);
}

void resizeSprite(Cube &cube, int id, int w, int h)
{
    uint8_t xb = -w;
    uint8_t yb = -h;

    uint16_t word = ((uint16_t)xb << 8) | yb;
    uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].mask_y)/2 +
                      sizeof(_SYSSpriteInfo)/2 * id ); 

    _SYS_vbuf_poke(&cube.vbuf.sys, addr, word);
}

void setSpriteImage(Cube &cube, int id, const Sifteo::PinnedAssetImage &assetImage, int frame)
{
	int tile = assetImage.index + ( frame * assetImage.height * assetImage.width );
    uint16_t word = VideoBuffer::indexWord(tile);
    uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].tile)/2 +
                      sizeof(_SYSSpriteInfo)/2 * id ); 

    _SYS_vbuf_poke(&cube.vbuf.sys, addr, word);
}