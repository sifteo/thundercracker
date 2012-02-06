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
volatile int x;

static void staticThing()
{
    x = 7;
}

void anotherThing()
{
    x = (int) &staticThing;
}

void siftmain()
{
    x = 5;
}