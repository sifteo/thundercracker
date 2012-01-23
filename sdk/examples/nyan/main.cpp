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

static Cube cubes[] = { Cube(0) };
static VidMode_BG0 vid[] = { VidMode_BG0(cubes[0].vbuf) };

static void init() {
    for (unsigned i=0; i<NUM_CUBES; i++) {
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
        if (done) break;
    }
}

static AudioChannel channel;

void siftmain() {
    init();
	channel.init();
	//channel.play(nyan, LoopRepeat);
	channel.play(Nyan, LoopRepeat);
    for (unsigned i=0; i<NUM_CUBES; i++) {
		vid[i].init();
	}
    int frame = 0;
    while (1) {
		float t = System::clock();
		for(unsigned i=0; i<NUM_CUBES; ++i) {
			vid[i].BG0_drawAsset(Vec2(0,0), Cat, frame);
		}
		while(System::clock() - t < 0.05f) {
			System::paint();
		}
		frame = (frame+1)%12;
    }
}
