/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

static void onCubeFound(_SYSCubeID c)
{
    printf("Found cube %d\n", c);
    _SYS_loadAssets(0, &GameAssets.sys);
}

void siftmain()
{
    _SYS_vectors.cubeFound = onCubeFound;
    _SYS_enableCubes(0x07);

    while (1) {
	System::draw();
    }
}
