/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

void siftmain()
{
    _SYS_enableCubes(1 << 0);
    _SYS_loadAssets(0, &GameAssets.sys);

    while (1) {
	System::draw();
    }
}
