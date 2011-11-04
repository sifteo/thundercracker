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
    vid[cid].BG0_textf(Vec2(2,2), Font, "Accel: %02x %02x", state.x + 0x80, state.y + 0x80);
}

static void onTilt(_SYSCubeID cid)
{
    _SYSTiltState state;
    _SYS_getTilt(cid, &state);
    vid[cid].BG0_textf(Vec2(2,4), Font, "Tilt: %d %d     ", state.x - 1, state.y - 1);
}

static void onShake(_SYSCubeID cid)
{
    _SYS_ShakeState state;
    _SYS_getShake(cid, &state);
    vid[cid].BG0_textf(Vec2(2,7), Font, "Shaking: %d", state);
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
        vid[i].clear(Font.tiles[0]);
    }
}

void siftmain()
{
    init();

	_SYS_vectors.cubeEvents.accelChange = onAccelChange;
    _SYS_vectors.cubeEvents.tilt = onTilt;
	_SYS_vectors.cubeEvents.shake = onShake;

    unsigned frame = 0;
    const unsigned rate = 2;

	onTilt(0);
	onShake(0);

    while (1) {
        float t = System::clock();
           
        System::paint();
    }
}
