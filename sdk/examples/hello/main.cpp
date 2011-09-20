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

static Cube cube(0);

void vPokeIndex(uint16_t addr, uint16_t tile)
{
    cube.vram.poke(addr, ((tile << 1) & 0xFE) | ((tile << 2) & 0xFE00));
}

void font_putc(uint8_t x, uint8_t y, char c)
{
    const uint8_t pitch = 18;
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
    cube.enable();
    cube.loadAssets(GameAssets);

    font_printf(0, 0, "Hello World!");
    font_printf(1, 3, "(>\")>  <(\"<)");

    for (unsigned y = 0; y < 5; y++)
	for (unsigned x = 0; x < 15; x++)
	    vPokeIndex(1+x + (10+y)*18, x + y*15 + 192);

    int x = 0;
    while (1) {
	static const char spinner[] = "-\\|/";
	x++;
	char c = spinner[x & 3];
		
	font_printf(1, 6, "%08x   %c%c%c", x, c, c, c);

	cube.vram.unlock();

	System::paint();
    }
}
