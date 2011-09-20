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

// XXX: Should be part of VRAM-mode-specific object
static void poke_index(uint16_t addr, uint16_t tile)
{
    cube.vbuf.poke(addr, ((tile << 1) & 0xFE) | ((tile << 2) & 0xFE00));
}

// XXX: Should have a higher level font object, supported by STIR.
static void font_putc(uint8_t x, uint8_t y, char c)
{
    const uint8_t pitch = 18;
    uint16_t index = (c - ' ') << 1;

    poke_index(x + pitch*y, index);
    poke_index(x + pitch*(y+1), index+1);
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

static void nextFrame()
{
    // XXX: cheesy frame trigger
    cube.vbuf.poke(511, cube.vbuf.sys.vram.words[511] ^ (_SYS_VF_TOGGLE << 8));
}

static void onAccelChange(_SYSCubeID cid)
{
    _SYSAccelState state;

    _SYS_getAccel(cid, &state);
    font_printf(2, 6, "Tilt: %02x %02x", state.x + 0x80, state.y + 0x80);

    // XXX: Cheesy panning hack
    int8_t px = -(state.x >> 4);
    int8_t py = -(state.y >> 4);
    if (px < 0) px += 18*8;
    if (py < 0) py += 18*8;
    cube.vbuf.poke(0x3fc/2, ((uint8_t)py << 8) | (uint8_t)px);

    nextFrame();
    cube.vbuf.unlock();
}

static void onAssetDone(_SYSCubeID cid)
{
    printf("Asset loading done\n");

    font_printf(2, 2, "Hello World!");

    // XXX: Drawing the logo manually, since there is no blit primitive yet
    for (unsigned y = 0; y < Logo.height; y++)
	for (unsigned x = 0; x < Logo.width; x++)
	    poke_index(1+x + (10+y)*18, Logo.tiles[x + y*Logo.width]);

    cube.vbuf.unlock();

    // Draw our accelerometer data now, plus on every change.
    _SYS_vectors.accelChange = onAccelChange;
    onAccelChange(cid);
}

void siftmain()
{
    // XXX: Mode-specific VRAM initialization
    cube.vbuf.init();
    memset(cube.vbuf.sys.vram.words, 0, sizeof cube.vbuf.sys.vram.words);
    cube.vbuf.sys.vram.mode = _SYS_VM_BG0;
    nextFrame();
    cube.vbuf.unlock();

    cube.enable();

    // XXX: Wait for cube to connect here...

    // Download assets, and continue when they're done.
    printf("Loading assets...\n");
    _SYS_vectors.assetDone = onAssetDone;
    cube.loadAssets(GameAssets);

    while (1) {
	System::paint();
    }
}
