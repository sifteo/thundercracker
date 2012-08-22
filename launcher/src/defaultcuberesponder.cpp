/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "defaultcuberesponder.h"
#include "shared.h"

using namespace Sifteo;

unsigned DefaultCubeResponder::called = 0;


void DefaultCubeResponder::init()
{
    ASSERT(cube.isDefined());
    position = velocity = vec(0, 0);
}

void DefaultCubeResponder::init(CubeID cid)
{
    cube = cid;
    init();
}

void DefaultCubeResponder::paint()
{
    ASSERT(cube.isDefined());

    // Call accounting to detect continuity of use.
    called++;

    motionUpdate();
}

void DefaultCubeResponder::motionUpdate()
{
    /*
     * This is the same motion algorithm used by our cube firmware:
     * It takes into account bounces, acceleration, and damping, and it
     * uses 16-bit fixed point math.
     *
     * The constants here have been tweaked a bit, since we're running
     * at a higher frame rate here and therefore our physics timestep
     * is shorter. We also don't have to worry about fitting anything
     * into 8 bits any more :)
     */

    const int kFPBits = 5;
    const int kDampingBits = 4;
    const int kDeadZone = 5;
    const int kRangeOfMotion = 24;
    const Int2 kPressVector = vec(0, 8);

    // Integer position becomes our new panning
    Int2 intPos = position >> kFPBits;
    Shared::video[cube].bg0.setPanning(intPos);

    // Let the logo portion bounce against the screen edges
    if (intPos.x > kRangeOfMotion) {
        position.x = kRangeOfMotion << kFPBits;
        velocity.x = -velocity.x;
    }
    if (intPos.x < -kRangeOfMotion) {
        position.x = -kRangeOfMotion << kFPBits;
        velocity.x = -velocity.x;
    }
    if (intPos.y > kRangeOfMotion) {
        position.y = kRangeOfMotion << kFPBits;
        velocity.y = -velocity.y;
    }
    if (intPos.y < -kRangeOfMotion) {
        position.y = -kRangeOfMotion << kFPBits;
        velocity.y = -velocity.y;
    }

    // Start out with a spring return force and damping force
    Int2 accel = intPos + (velocity >> kDampingBits);

    // Add 'press' force
    if (cube.isTouching())
        accel += kPressVector;

    // Add tilt, if we're outside the dead zone
    Int2 cubeAccel = cube.accel().xy();
    if (cubeAccel.lenManhattan() >= kDeadZone)
        accel += cubeAccel;

    // Integrate
    Int2 fpVelocity = velocity - accel;
    Int2 fpPosition = position + fpVelocity;
    velocity = fpVelocity;
    position = fpPosition;

//    if (intPos.x > /
#if 0


    Sifteo::Short2 position;
    Sifteo::Short2 velocity;
    Sifteo::CubeID cube;


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

     ;    disc_logo_x += (disc_logo_dx -= x + (disc_logo_dx >> FP_BITS) + ack_data.accel[0]);
        ;    disc_logo_y += (disc_logo_dy -= -BATTERY_HEIGHT + y + (disc_logo_dy >> FP_BITS) + ack_data.accel[1]);

    Shared::video[cube].bg0.setPanning(position - cube.accel().xy()/2.f);
#endif
}
