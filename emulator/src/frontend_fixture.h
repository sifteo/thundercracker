/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _FRONTEND_FIXTURE_H
#define _FRONTEND_FIXTURE_H

#include <Box2D/Box2D.h>
#include "cube_hardware.h"

class FrontendCube;
class FrontendMC;

struct FixtureData {
    enum Type {
        T_CUBE = 0,
        T_NEIGHBOR,
        T_MC
    };

    Type type;

    union {
        FrontendCube *cube;
        FrontendMC *mc;
    } ptr;

    Cube::Neighbors::Side side;
};

#endif
