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

class FrontendCube;
class FrontendMC;

struct FixtureData {
    enum Type {
        T_CUBE = 0,
        T_MC,
        T_CUBE_NEIGHBOR,
        T_MC_NEIGHBOR,
    };

    Type type;

    union {
        FrontendCube *cube;
        FrontendMC *mc;
    } ptr;

    unsigned side;
};

#endif
