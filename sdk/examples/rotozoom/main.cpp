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
        float zoom = 0.5 + sinf(frame * 0.001) * 0.2;

        float s = sinf(angle) * zoom;
        float c = cosf(angle) * zoom;

        float x = 0.5; // - c*64/zoom + s*64/zoom;
        float y = 0.5; // - s*64/zoom - c*64/zoom;

        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.cx)/2, 0x100 * x);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.cx)/2, 0x100 * y);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.xx)/2, 0x100 * c);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.xy)/2, 0x100 * s);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.yx)/2, 0x100 * -s);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.yy)/2, 0x100 * c);

        /*
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.cx)/2, 0x100 * 0);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.cx)/2, 0x100 * 0);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.xx)/2, 0x100 * 1);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.xy)/2, 0x100 * 0);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.yx)/2, 0x100 * 0);
        cube.vbuf.poke(offsetof(_SYSVideoRAM, bg2_affine.yy)/2, 0x100 * 1);
        */

        System::paint();
    }
}
