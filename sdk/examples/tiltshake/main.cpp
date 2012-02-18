/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

#ifndef NUM_CUBES
#  define NUM_CUBES 1
#endif

static Cube cubes[] = { Cube(0), Cube(1) };
static VidMode_BG0 vid[] = { VidMode_BG0(cubes[0].vbuf), VidMode_BG0(cubes[1].vbuf) };

static void onAccelChange(void *context, _SYSCubeID cid)
{
    _SYSAccelState state;
    _SYS_getAccel(cid, &state);

    String<18> str;
    str << "Accel: " << Hex(state.x + 0x80, 2) << " " << Hex(state.y + 0x80, 2);
    vid[cid].BG0_text(Vec2(2,2), Font, str);
}

static void onTilt(void *context, _SYSCubeID cid)
{
    _SYSTiltState state;
    _SYS_getTilt(cid, &state);

    String<18> str;
    str << "Tilt: " << (state.x - 1) << " " << (state.y - 1) << "     ";
    vid[cid].BG0_text(Vec2(2,4), Font, str);
}

static void onShake(void *context, _SYSCubeID cid)
{
    _SYSShakeState state;
    _SYS_getShake(cid, &state);

	if( state == SHAKING )
		vid[cid].BG0_drawAsset(Vec2(0,0), Shake);
	else
		vid[cid].clear(Font.tiles[0]);
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

    _SYS_setVector(_SYS_CUBE_ACCELCHANGE, (void*) onAccelChange, NULL);
    _SYS_setVector(_SYS_CUBE_TILT, (void*) onTilt, NULL);
    _SYS_setVector(_SYS_CUBE_SHAKE, (void*) onShake, NULL);

    unsigned frame = 0;
    const unsigned rate = 2;

	onTilt(NULL, 0);
	onShake(NULL, 0);

    while (1) {
        System::paint();
    }
}