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

static void poke_index(uint16_t addr, uint16_t tile)
{
    cube.vbuf.poke(addr, ((tile << 1) & 0xFE) | ((tile << 2) & 0xFE00));
}

void siftmain()
{
    cube.vbuf.init();

    memset(cube.vbuf.sys.vram.words, 0, sizeof cube.vbuf.sys.vram.words);
    cube.vbuf.sys.vram.mode = _SYS_VM_BG0_BG1;
    cube.vbuf.sys.vram.flags = _SYS_VF_CONTINUOUS;
    cube.vbuf.sys.vram.num_lines = 128;

    for (unsigned y = 0; y < Background.height; y++)
        for (unsigned x = 0; x < 18; x++)
            poke_index(offsetof(_SYSVideoRAM, bg0_tiles)/2 +
                       x + y*18, Background.tiles[x + y*Background.width]);

    for (unsigned y = 0; y < Overlay.height; y++)
        for (unsigned x = 0; x < Overlay.width; x++)
            poke_index(offsetof(_SYSVideoRAM, bg1_tiles)/2 +
                       x + y*Overlay.width,
                       Overlay.tiles[x + y*Overlay.width]);

    for (unsigned y = 0; y < Overlay.height; y++)
        cube.vbuf.sys.vram.bg1_bitmap[y+2] = ((1 << Overlay.width) - 1) << 4; 

    cube.enable();
    cube.loadAssets(GameAssets);

    unsigned t = 0;

    while (1) {
        t++;
        if (t == (18*8))
            t = 0;

        cube.vbuf.pokeb(offsetof(_SYSVideoRAM, bg0_x), t);
        cube.vbuf.unlock();

        System::paint();
    }
}
