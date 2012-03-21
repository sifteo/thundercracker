/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

static VideoBuffer vbuf;
static AudioChannel channel;

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
            rom.BG0_progressBar(Vec2(0,7),
                cubes[i].assetProgress(GameAssets, VidMode_BG0::LCD_width), 2);
            if (!cubes[i].assetDone(GameAssets))
                done = false;
        }
        System::paint();
        if (done) break;
    }
}

void main() {
    init();
	channel.init();
	channel.play(Nyan, LoopRepeat);

    for (unsigned i=0; i<NUM_CUBES; i++)
        VidMode_BG0(cubes[i].vbuf).init();

    while (1) {
        unsigned frame = SystemTime::now().cycleFrame(0.5, Cat.frames);

    	for (unsigned i=0; i<NUM_CUBES; ++i) {
            VidMode_BG0 vid(cubes[i].vbuf);
            vid.BG0_drawAsset(Vec2(0,0), Cat, frame);
        }

    	System::paint();
    }
}
