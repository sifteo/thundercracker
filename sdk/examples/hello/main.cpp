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

#ifndef NUM_CUBES
#  define NUM_CUBES 1
#endif

static Cube cubes[] = { Cube(0), Cube(1) };
static VidMode_BG0 vid[] = { VidMode_BG0(cubes[0].vbuf), VidMode_BG0(cubes[1].vbuf) };

static void onAccelChange(_SYSCubeID cid)
{
    _SYSAccelState state;
    _SYS_getAccel(cid, &state);
    vid[cid].BG0_textf(Vec2(2,4), Font, "Tilt: %02x %02x", state.x + 0x80, state.y + 0x80);
    vid[cid].BG0_setPanning(Vec2(-state.x/2, -state.y/2));
}

static void init()
{
    for (unsigned i = 0; i < NUM_CUBES; i++) {
        cubes[i].enable();
        cubes[i].loadAssets(GameAssets);

        VidMode_BG0_ROM rom(cubes[i].vbuf);
        rom.init();
        rom.BG0_text(Vec2(1,1), "Loading...");
    }

    for (;;) {
        bool done = true;

        for (unsigned i = 0; i < NUM_CUBES; i++) {
            VidMode_BG0_ROM rom(cubes[i].vbuf);
            rom.BG0_progressBar(Vec2(0,7), cubes[i].assetProgress(GameAssets, VidMode_BG0::LCD_width), 2);
            if (!cubes[i].assetDone(GameAssets))
                done = false;
        }

        System::paint();

        if (done)
            break;
    }

    for (unsigned i = 0; i < NUM_CUBES; i++) {
        vid[i].init();
    }
}

void siftmain()
{
    init();

    _SYS_vectors.accelChange = onAccelChange;

    for (unsigned i = 0; i < NUM_CUBES; i++) {
        vid[i].BG0_text(Vec2(2,1), Font, "Hello World!");
        vid[i].BG0_drawAsset(Vec2(1,10), Logo);
        onAccelChange(cubes[i].id());
    } 

    unsigned frame = 0;
    const unsigned rate = 2;

    while (1) {
        float t = System::clock();

        for (unsigned i = 0; i < NUM_CUBES; i++) {
            vid[i].BG0_textf(Vec2(2,6), Font, "Time: %4u.%u", (int)t, (int)(t*10) % 10);
            vid[i].BG0_drawAsset(Vec2(11,9), Kirby, frame >> rate);
        }

        if (++frame == Kirby.frames << rate)
            frame = 0;
            
        System::paint();
    }
}
