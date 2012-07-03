/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "cuberange.h"
#include <sifteo.h>
using namespace Sifteo;


CubeRange::CubeRange(const _SYSMetadataCubeRange *cr)
{
    if (cr)
        sys = *cr;
    else
        bzero(sys);
}

bool CubeRange::isValid()
{
    return sys.minCubes <= sys.maxCubes && sys.minCubes <= _SYS_NUM_CUBE_SLOTS;
}

CubeSet CubeRange::initMinimum()
{
    CubeSet cubes(0, sys.minCubes);
    SCRIPT_FMT(LUA, "System():setOptions{ numCubes = %d }", sys.minCubes);
    _SYS_enableCubes(cubes);
    return cubes;
}
