/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <sifteo.h>
#include "audio.gen.h"

using namespace Sifteo;

#ifndef NUM_CUBES
#  define NUM_CUBES 1
#endif

static Cube cubes[] = { Cube(0) };
static VidMode_BG0 vid[] = { VidMode_BG0(cubes[0].vbuf) };

static void init() {
    for (unsigned i=0; i<NUM_CUBES; i++) {
        cubes[i].enable();
        VidMode_BG0_ROM rom(cubes[i].vbuf);
    }
}

const int NUM_CHANNELS = 3;
static AudioChannel channels[NUM_CHANNELS];


const _SYSAudioModule *SOUNDS[NUM_CHANNELS] =
{
  &astrokraut,
	&bubble_pop_02,
		&match2,
};

void siftmain() {
    init();

	for( int i = 0; i < NUM_CHANNELS; i++ )
	{
		channels[i].init();
		channels[i].play(*SOUNDS[i], LoopRepeat);
	}
    
	while(1)
	{
		System::paint();
	}
}