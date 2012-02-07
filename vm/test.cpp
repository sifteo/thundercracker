/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>

using namespace Sifteo;

#ifndef NUM_CUBES
#  define NUM_CUBES 1
#endif

static Cube cubes[NUM_CUBES];

void siftmain()
{
    for (unsigned i = 0; i < NUM_CUBES; i++) {
        //cubes[i].enable(i);

        VidMode_BG0_ROM vid(cubes[i].vbuf);
        vid.clear();
        vid.set();
        //vid.BG0_setPanning(Vec2(0,0));
        //vid.setWindow(0, vid.LCD_height);
    }

    while (1) {
       System::paint();
    }
}