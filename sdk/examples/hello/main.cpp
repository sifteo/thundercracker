/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>   // Only available on the simulator, of course
#include <sifteo.h>

static void onCubeFound(CubeID c)
{
    printf("Found cube %d\n", c);
}

void siftmain()
{
    SYS_vectors.cubeFound = onCubeFound;
    SYS_setEnabledCubes(0x07);
}
