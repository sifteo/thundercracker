/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

static Cube cube(0);

static void vPokeIndex(uint16_t addr, uint16_t tile)
{
    cube.vram.poke(addr, ((tile << 1) & 0xFE) | ((tile << 2) & 0xFE00));
}

static void font_putc(uint8_t x, uint8_t y, char c)
{
    const uint8_t pitch = 18;
    uint16_t index = (c - ' ') << 1;

    vPokeIndex(x + pitch*y, index);
    vPokeIndex(x + pitch*(y+1), index+1);
}

static void font_printf(uint8_t x, uint8_t y, const char *fmt, ...)
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

static void onAssetDone(struct _SYSAssetGroup *group)
{
    printf("Asset loading done\n");

    font_printf(0, 0, "Hello World!");

    for (unsigned y = 0; y < Logo.height; y++)
	for (unsigned x = 0; x < Logo.width; x++)
	    vPokeIndex(1+x + (10+y)*18, Logo.tiles[x + y*Logo.width]);

    cube.vram.unlock();
}

void siftmain()
{
    _SYS_vectors.assetDone = onAssetDone;

    cube.vram.init();
    memset(cube.vram.sys.words, 0, sizeof cube.vram.sys.words);
    cube.vram.sys.words[402] = 0xFFFF;
    cube.vram.sys.words[405] = 0xFFFF;
    cube.vram.unlock();

    cube.enable();
    cube.loadAssets(GameAssets);

    while (1) {
	System::paint();
    }
}
