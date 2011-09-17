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

void vPokeIndex(uint16_t addr, uint16_t tile)
{
    printf("Poke %04x %04x\n", addr, tile);
    vPoke(addr, ((tile << 1) & 0xFE) | ((tile << 2) & 0xFE00));
}

void putchar(uint8_t x, uint8_t y, char c)
{
    const uint8_t pitch = 20;
    uint16_t index = (c - ' ') << 1;

    vPokeIndex(x + pitch*y, index);
    vPokeIndex(x + pitch*(y+1), index+1);
}

void print(uint8_t x, uint8_t y, const char *str)
{
    while (*str)
	putchar(x++, y, *(str++));
}

void siftmain()
{
    _SYS_enableCubes(1 << 0);
    _SYS_loadAssets(0, &GameAssets.sys);
    _SYS_setVideoBuffer(0, &vbuf);

    print(0, 0, "Hello World!");
	
    while (1) {
	System::draw();
    }
}
