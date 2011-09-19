/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include <stdarg.h>

#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

static _SYSVideoBuffer vbuf;

void markCM(uint16_t addr)
{
    Atomic::Or(vbuf.cm1[addr >> 5], 0x80000000 >> (addr & 31));
    Atomic::Or(vbuf.cm32, 0x80000000 >> (addr >> 5));
}

void vPoke(uint16_t addr, uint16_t word)
{
    if (vbuf.words[addr] != word) {
	vbuf.words[addr] = word;
	markCM(addr);
    }
}

void vPokeIndex(uint16_t addr, uint16_t tile)
{
    vPoke(addr, ((tile << 1) & 0xFE) | ((tile << 2) & 0xFE00));
}

void font_putc(uint8_t x, uint8_t y, char c)
{
    const uint8_t pitch = 20;
    uint16_t index = (c - ' ') << 1;

    vPokeIndex(x + pitch*y, index);
    vPokeIndex(x + pitch*(y+1), index+1);
}

void font_printf(uint8_t x, uint8_t y, const char *fmt, ...)
{
    char buf[128];
    char *p = buf;
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf - 1, fmt, ap);
    buf[sizeof buf - 1] = 0;
    while (*p)
	font_putc(x++, y, *(p++));
    va_end(ap);
}

void siftmain()
{
    for (unsigned i = 0; i < 20*20; i++)
	markCM(i);

    _SYS_enableCubes(1 << 0);
    _SYS_loadAssets(0, &GameAssets.sys);
    _SYS_setVideoBuffer(0, &vbuf);

    font_printf(0, 0, "Hello World!");
    font_printf(1, 3, "(>\")>  <(\"<)");

    int x = 0;
    while (1) {
	static const char spinner[] = "-\\|/";
	x++;
	font_printf(1, 6, "%08x   %c", x, spinner[(x >> 10) & 3]);
	System::draw();
    }
}
