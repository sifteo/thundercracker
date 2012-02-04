/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>

using namespace Sifteo;

#ifndef NUM_CUBES
#  define NUM_CUBES 3
#endif

static Cube cubes[NUM_CUBES];

void siftmain()
{
    for (unsigned i = 0; i < NUM_CUBES; i++) {
        //cubes[i].enable(i);

        //VidMode_BG0_ROM vid(cubes[i].vbuf);
        //vid.init();
    }

    while (1) {
       System::paint();
    }
}