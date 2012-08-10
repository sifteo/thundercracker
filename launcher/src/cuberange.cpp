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

bool CubeRange::isEmpty()
{
    return sys.minCubes == 0;
}

bool CubeRange::isValid()
{
    return sys.minCubes <= sys.maxCubes && sys.minCubes <= _SYS_NUM_CUBE_SLOTS;
}

void CubeRange::set()
{
    LOG("LAUNCHER: Using cube range [%d,%d]\n", sys.minCubes, sys.maxCubes);
    System::setCubeRange(sys.minCubes, sys.maxCubes);
}
