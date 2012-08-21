/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "defaultcuberesponder.h"
#include "shared.h"

using namespace Sifteo;

const Float2 DefaultCubeResponder::kRest = vec(0.f, 0.f);
const float DefaultCubeResponder::kRate = 0.05f;
const float DefaultCubeResponder::kMag = 20.f;
const float DefaultCubeResponder::kShakeCount = 5.f;
const float DefaultCubeResponder::kDownTarget = -8.f;
const float DefaultCubeResponder::kDownRate = 0.2f;
const float DefaultCubeResponder::kUpRate = 0.35f;

unsigned DefaultCubeResponder::called = 0;

void DefaultCubeResponder::_init()
{
    ASSERT(cube.isDefined());

    u = 0.f;
    position = kRest;
    needInit = false;
}

void DefaultCubeResponder::init(CubeID cid)
{
    cube = cid;
    init();
}

void DefaultCubeResponder::paint()
{
    ASSERT(cube.isDefined());

    // Delayed initialization.
    if (needInit) _init();

    // Call accounting to detect continuity of use.
    called++;

    // Basic shake touch response
    if (cube.isTouching()) {
        // Initialize animation timer
        u = 1.f;

        position.x = (1-kDownRate) * position.x  + kDownRate * kRest.x;
        position.y  = (1-kDownRate) * position.y  + kDownRate * kDownTarget;
    } else if (u > 0) {
        // Timer countdown
        u -= kRate;
        if (u < 0) u = 0;

        position.x = kRest.x + kMag * u * sin(kShakeCount * 3.14159f * u);
        position.y = (1 - kUpRate) * position.y + kUpRate * kRest.y;
    }
    Shared::video[cube].bg0.setPanning(position - cube.accel().xy()/2.f);
}
