/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static const unsigned gNumCubes = 4;
static VideoBuffer vid[CUBE_ALLOCATION];

static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(GameAssets);

static Metadata M = Metadata()
    .title("Pantera")
    .cubeRange(gNumCubes);

CubeID ctilt = 0;
CubeID cshake = 1;
CubeID cpress = 2;
CubeID cno = 3;

const AssetImage* icons[] = { &IconTilt, &IconShake, &IconPress, &IconNo };

void main()
{
    for (CubeID cube=0; cube < gNumCubes; ++cube) {
        const int mid = 48/8;
        vid[cube].initMode(BG0);
        vid[cube].bg0.image(vec(0,0), Background);
        vid[cube].bg0.image(vec(mid,mid), *icons[cube]);
        vid[cube].bg0.setPanning(vec(8,8));

        cube.enable();
        vid[cube].attach(cube);
    }

    const Float2 rest = vec(8.f, 8.f);

    // shake params
    const int iters = 12;
    const float bias = -0.05f;
    const float dt = 0.1f;
    const float k = 0.15f;
    const float damp = 0.975f;
    static Float2 shakeVelocity = vec(0.f,0.f);
    static Float2 shakePosition = rest;

    // press params
    float pressu = 0.f;
    float pressrate = 0.075f;
    static Float2 pressPosition = rest;
    static float downTarget = -8.f;
    const float downRate = 0.2f;
    const float upRate = 0.35f;

    // no params
    float u = 0.f;
    float rate = 0.05f;
    float mag = 20.f;
    float noShakeCount = 5.f;
    Float2 noPosition = rest;

    while (1) {

        // update tilt
        vid[ctilt].bg0.setPanning(rest - ctilt.accel().xy()/2.f);

        // update shake
        for(int i=0; i<iters; ++i) {
            shakeVelocity += dt * bias * cshake.accel().xy();
            shakeVelocity += dt * k * (rest - shakePosition);
            shakePosition += dt * shakeVelocity;
            shakeVelocity *= damp;
        }
        vid[cshake].bg0.setPanning(shakePosition);
        
        // update press
        if (cpress.isTouching()) {
            pressPosition.x = (1-downRate) * pressPosition.x  + downRate * rest.x;
            pressPosition.y = (1-downRate) * pressPosition.y + downRate * downTarget;
            //pressu = (pressPosition.y - rest.y) / (downTarget - rest.y);
            pressu = 1.f;
        } else {
            pressu -= pressrate;
            if (pressu < 0) { pressu = 0; }
            pressPosition.y = 8.f + 16.f * pressu * sin(1.5f * 3.14159f * pressu);
        }
        vid[cpress].bg0.setPanning(pressPosition);

        // update no
        if (cno.isTouching()) {
            u = 1.f;
            noPosition.x = (1-downRate) * noPosition.x  + downRate * rest.x;
            noPosition.y  = (1-downRate) * noPosition.y  + downRate * downTarget;
        } else {
            u -= rate;
            if (u < 0) { u = 0; }
            noPosition.x = rest.x + mag * u * sin(noShakeCount * 3.14159f * u);
            noPosition.y = (1-upRate) * noPosition.y + upRate * rest.y;

        }
        vid[cno].bg0.setPanning(noPosition);

   	    System::paint();
    }
}
