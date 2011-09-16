/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

static _SYSVideoBuffer vbuf;

void vPoke(uint16_t addr, uint16_t word)
{
    vbuf.words[addr] = word;
    Atomic::Or(vbuf.cm1[addr >> 5], 0x80000000 >> (addr & 31));
    Atomic::Or(vbuf.cm32, 0x80000000 >> (addr >> 5));
}

void siftmain()
{
    _SYS_enableCubes(1 << 0);
    _SYS_loadAssets(0, &GameAssets.sys);
    _SYS_setVideoBuffer(0, &vbuf);

    uint16_t x = 0;

    while (1) {
	x++;
	vPoke(x & 511, x);

	System::draw();
    }
}
