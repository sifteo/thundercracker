/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

static Cube cube(0);

static void progress_bar(uint16_t addr, int pixelWidth)
{
    while (pixelWidth > 0) {
        if (pixelWidth < 8)  {
            cube.vbuf.pokei(addr, 0x085f + pixelWidth);
            break;

        } else {
            cube.vbuf.pokei(addr, 0x09ff);
            addr++;
            pixelWidth -= 8;
        }
    }
}

void siftmain()
{
    cube.vbuf.init();

    memset(cube.vbuf.sys.vram.words, 0, sizeof cube.vbuf.sys.vram.words);
    cube.vbuf.sys.vram.mode = _SYS_VM_BG0_ROM;
    cube.vbuf.sys.vram.num_lines = 128;
    cube.enable();

    cube.loadAssets(GameAssets);

    for (;;) {
        unsigned progress = GameAssets.sys.cubes[0].progress;
        unsigned length = GameAssets.sys.hdr->dataSize;

        progress_bar(0, progress * 128 / length);
        System::paint();

        if (progress == length)
            break;
    }

    cube.vbuf.pokeb(offsetof(_SYSVideoRAM, mode), _SYS_VM_BG2);

    for (unsigned y = 0; y < Background.height; y++)
        for (unsigned x = 0; x < Background.width; x++)
            cube.vbuf.pokei(offsetof(_SYSVideoRAM, bg2_tiles)/2 +
                            x + y*16, Background.tiles[x + y * Background.width]);

    unsigned frame = 0;

    while (1) {
        AffineMatrix m = AffineMatrix::identity();
        
        /*
         * Build an affine transformation for this frame
         */

        m.translate(Background.width * 8 / 2,
                    Background.height * 8 / 2);

        m.scale(fabs(sinf(frame * 0.008) * 2));
        m.rotate(frame * 0.03);

        m.translate(-64, -64);

        /*
         * Orient the screen such that the X axis is major.
         */

        uint8_t flags = cube.vbuf.peekb(offsetof(_SYSVideoRAM, flags));

        if (fabs(m.xy) > fabs(m.xx)) {
            flags |= _SYS_VF_XY_SWAP;
            m *= AffineMatrix(0, 1, 0,
                              1, 0, 0);
        } else {
            flags &= ~_SYS_VF_XY_SWAP;
        }

        if (flags != cube.vbuf.peekb(offsetof(_SYSVideoRAM, flags))) {
            System::finish();
            cube.vbuf.pokeb(offsetof(_SYSVideoRAM, flags), flags);
        }

        /*
         * Convert the matrix to fixed-point
         */

        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.cx)/2, 0x100 * m.cx);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.cy)/2, 0x100 * m.cy);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.xx)/2, 0x100 * m.xx);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.xy)/2, 0x100 * m.xy);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.yx)/2, 0x100 * m.yx);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.yy)/2, 0x100 * m.yy);

        System::paint();
        frame++;
    }
}
