/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include <sifteo.h>
#include <math.h>
#include "assets.gen.h"

using namespace Sifteo;

static Cube cube(0);

struct AffineMatrix {
    /**
     * An augmented 3x2 matrix, for doing 2D affine transforms.
     *
     *  [ xx  yx  cx ]
     *  [ xy  yy  cy ]
     *  [  0   0   1 ]
     *
     * The way we use affine transforms for background scaling are
     * very similiar to the mechanism used by the GameBoy Advance
     * PPU. There's a great tutorial on this at:
     *
     * http://www.coranac.com/tonc/text/affine.htm
     */

    float cx, cy;
    float xx, xy;
    float yx, yy;
    
    static AffineMatrix identity() {
        AffineMatrix m;

        m.cx = 0;
        m.cy = 0;
        m.xx = 1;
        m.xy = 0;
        m.yx = 0;
        m.yy = 1;

        return m;
    }

    static AffineMatrix scaling(float s) {
        AffineMatrix m;
        float inv_s = 1.0f / s;

        m.cx = 0;
        m.cy = 0;
        m.xx = inv_s;
        m.xy = 0;
        m.yx = 0;
        m.yy = inv_s;

        return m;
    }

    static AffineMatrix translation(float x, float y) {
        AffineMatrix m;

        m.cx = x;
        m.cy = y;
        m.xx = 1;
        m.xy = 0;
        m.yx = 0;
        m.yy = 1;

        return m;
    }

    static AffineMatrix rotation(float angle) {
        AffineMatrix m;
        float s = sinf(angle);
        float c = cosf(angle);

        m.cx = 0;
        m.cy = 0;
        m.xx = c;
        m.xy = s;
        m.yx = -s;
        m.yy = c;

        return m;
    }

    void operator*= (float scale) {
        cx *= scale;
        cy *= scale;
        xx *= scale;
        xy *= scale;
        yx *= scale;
        yy *= scale;
    }

    void operator*= (const AffineMatrix &m) {
        AffineMatrix n;

        n.cx = xx*m.cx + yx*m.cy + cx;
        n.cy = xy*m.cx + yy*m.cy + cy;
        n.xx = xx*m.xx + yx*m.xy;
        n.xy = xy*m.xx + yy*m.xy;
        n.yx = xx*m.yx + yx*m.yy;
        n.yy = xy*m.yx + yy*m.yy;

        *this = n;
    }

    void translate(float x, float y) {
        *this *= translation(x, y);
    }

    void rotate(float angle) {
        *this *= rotation(angle);
    }
    
    void scale(float s) {
        *this *= scaling(s);
    }
};

static void poke_index(uint16_t addr, uint16_t tile)
{
    cube.vbuf.poke(addr, ((tile << 1) & 0xFE) | ((tile << 2) & 0xFE00));
}

void siftmain()
{
    cube.vbuf.init();

    memset(cube.vbuf.sys.vram.words, 0, sizeof cube.vbuf.sys.vram.words);
    cube.vbuf.sys.vram.mode = _SYS_VM_BG2;
    cube.vbuf.sys.vram.flags = _SYS_VF_CONTINUOUS;
    cube.vbuf.sys.vram.num_lines = 128;

    for (unsigned y = 0; y < Background.height; y++)
        for (unsigned x = 0; x < Background.width; x++)
            poke_index(offsetof(_SYSVideoRAM, bg2_tiles)/2 +
                       x + y*16, Background.tiles[x + y * Background.width]);

    cube.enable();
    cube.loadAssets(GameAssets);

    unsigned frame = 0;

    while (1) {
        frame++;

        float angle = frame * 0.01;

        AffineMatrix m = AffineMatrix::identity();
        
        m.translate(Background.width * 8 / 2,
                    Background.height * 8 / 2);
        m.scale(1.5);
        m.rotate(angle);
        m.translate(-64, -64);

        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.cx)/2, 0x100 * m.cx);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.cy)/2, 0x100 * m.cy);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.xx)/2, 0x100 * m.xx);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.xy)/2, 0x100 * m.xy);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.yx)/2, 0x100 * m.yx);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.yy)/2, 0x100 * m.yy);

        System::paint();
    }
}
