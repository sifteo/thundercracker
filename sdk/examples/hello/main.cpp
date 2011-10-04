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

// XXX: Should be part of SDK
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

// XXX: Should have a higher level font object, supported by STIR.
static void font_putc(uint8_t x, uint8_t y, char c)
{
    const uint8_t pitch = 18;
    uint16_t index = (c - ' ') << 1;

    cube.vbuf.pokei(x + pitch*y, index);
    cube.vbuf.pokei(x + pitch*(y+1), index+1);
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

static void onAccelChange(_SYSCubeID cid)
{
    _SYSAccelState state;

    _SYS_getAccel(cid, &state);
    font_printf(2, 5, "Tilt: %02x %02x", state.x + 0x80, state.y + 0x80);

    int8_t px = -(state.x >> 1);
    int8_t py = -(state.y >> 1);
    if (px < 0) px += 18*8;
    if (py < 0) py += 18*8;
    cube.vbuf.pokeb(offsetof(_SYSVideoRAM, bg0_x), px);
    cube.vbuf.pokeb(offsetof(_SYSVideoRAM, bg0_y), py);
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

    cube.vbuf.pokeb(offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0);
    for (unsigned i = 0; i < 18*18; i++)
        cube.vbuf.pokei(i, 0);

    font_printf(2, 2, "Hello World!");

    // XXX: Drawing the logo manually, since there is no blit primitive yet
    for (unsigned y = 0; y < Logo.height; y++)
        _SYS_vbuf_writei(&cube.vbuf.sys, (10+y)*18 + 1, &Logo.tiles[y*Logo.width], 0, Logo.width);

    // Draw our accelerometer data now, plus on every change.
    _SYS_vectors.accelChange = onAccelChange;
    onAccelChange(cube.id());

    while (1) {
        float t = System::clock();
        font_printf(2, 7, "Time: %4u.%u", (int)t, (int)(t*10) % 10);
        System::paint();
    }
}
