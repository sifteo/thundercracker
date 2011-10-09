/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "cube_neighbors.h"
#include "cube_hardware.h"

namespace Cube {


void Neighbors::attachCubes(Cube::Hardware *cubes)
{
    /*
     * The system is giving us a pointer to the other cubes, so
     * that we can scan their neighbor state on demand.
     */ 
    otherCubes = cubes;
}

uint8_t Neighbors::checkNeighbor(unsigned otherCube, uint8_t otherSideBit)
{
    // Is this other cube pulsing right this instant?
    if (otherCubes[otherCube].neighbors.driveEdge & otherSideBit)
        return PIN_IN;
    else 
        return 0;
}


};  // namespace Cube
