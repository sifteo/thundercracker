/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker launcher
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
    const int kDampingBits = 3;
    const int kDeadZone = 5;
    const int kRangeOfMotion = 24;
    const Int2 kPressVector = vec(0, -5) << kFPBits;
    const int kNeighborMagnetism = 6 << kFPBits;

    // Integer position becomes our new panning
    Int2 intPos = fpRound(position, kFPBits);
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
    Int2 accel = intPos + fpTrunc(velocity, kDampingBits);

    // Jump when pressed
    bool touching = cube.isTouching();
    if (touching && !wasTouching) {
        accel += kPressVector;
    }
    wasTouching = touching;

    // Neighbors are magnetic
    Neighborhood nbr(cube);
    if (nbr.hasNeighborAt(TOP))     accel.y -= kNeighborMagnetism;
    if (nbr.hasNeighborAt(LEFT))    accel.x -= kNeighborMagnetism;
    if (nbr.hasNeighborAt(BOTTOM))  accel.y += kNeighborMagnetism;
    if (nbr.hasNeighborAt(RIGHT))   accel.x += kNeighborMagnetism;

    // Add tilt, if we're outside the dead zone
    Int2 cubeAccel = cube.accel().xy();
    if (cubeAccel.lenManhattan() >= kDeadZone)
        accel += cubeAccel;

    // Integrate
    Int2 fpVelocity = velocity - accel;
    Int2 fpPosition = position + fpVelocity;
    velocity = fpVelocity;
    position = fpPosition;
}
