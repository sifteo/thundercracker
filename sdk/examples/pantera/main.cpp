#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

#define NUM_CUBES 4

static Cube cubes[NUM_CUBES];
Cube& ctilt = cubes[0];
Cube& cshake = cubes[1];
Cube& cpress = cubes[2];
Cube& cno = cubes[3];
VidMode_BG0 gtilt = VidMode_BG0(cubes[0].vbuf);
VidMode_BG0 gshake = VidMode_BG0(cubes[1].vbuf);
VidMode_BG0 gpress = VidMode_BG0(cubes[2].vbuf);
VidMode_BG0 gno = VidMode_BG0(cubes[3].vbuf);
const AssetImage* icons[] = { &IconTilt, &IconShake, &IconPress, &IconNo };

void main() {
    for (unsigned i=0; i<NUM_CUBES; i++) {
        LOG(("ENABLE %d\n", i));
        cubes[i].enable(i);
        cubes[i].loadAssets(GameAssets);
        VidMode_BG0_ROM rom(cubes[i].vbuf);
        rom.init();
        rom.BG0_text(vec(1,1), "Loading...");
    }
    bool done = false;
    while (!done) {
        done = true;
        for (unsigned i = 0; i < NUM_CUBES; i++) {
            VidMode_BG0_ROM rom(cubes[i].vbuf);
            rom.BG0_progressBar(vec(0,7), cubes[i].assetProgress(GameAssets, VidMode_BG0::LCD_width), 2);
            if (!cubes[i].assetDone(GameAssets)) {
                done = false;
            }
        }
        System::paint();
    }

    System::paintSync();
    int mid = 48/8;
    for (unsigned i=0; i<NUM_CUBES; i++) {
        VidMode_BG0 g(cubes[i].vbuf);
        g.init();
        g.BG0_drawAsset(vec(0,0), Background);
        g.BG0_drawAsset(vec(mid,mid), *icons[i]);
        g.BG0_setPanning(vec(8,8));
    }
    System::paintSync();
    for (unsigned i=0; i<NUM_CUBES; ++i) { cubes[i].vbuf.touch(); }
    System::paintSync();
    for (unsigned i=0; i<NUM_CUBES; ++i) { cubes[i].vbuf.touch(); }
    System::paintSync();
    
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
        gtilt.BG0_setPanning(rest - ctilt.physicalAccel()/2.f);

        // update shake
        for(int i=0; i<iters; ++i) {
            shakeVelocity += dt * bias * cshake.physicalAccel();
            shakeVelocity += dt * k * (rest - shakePosition);
            shakePosition += dt * shakeVelocity;
            shakeVelocity *= damp;
        }
        gshake.BG0_setPanning(shakePosition);
        
        // update press
        if (cpress.touching()) {
            pressPosition.x = (1-downRate) * pressPosition.x  + downRate * rest.x;
            pressPosition.y = (1-downRate) * pressPosition.y + downRate * downTarget;
            //pressu = (pressPosition.y - rest.y) / (downTarget - rest.y);
            pressu = 1.f;
        } else {
            pressu -= pressrate;
            if (pressu < 0) { pressu = 0; }
            pressPosition.y = 8.f + 16.f * pressu * sin(1.5f * 3.14159f * pressu);
        }
        gpress.BG0_setPanning(pressPosition);


        // update no
        if (cno.touching()) {
            u = 1.f;
            noPosition.x = (1-downRate) * noPosition.x  + downRate * rest.x;
            noPosition.y  = (1-downRate) * noPosition.y  + downRate * downTarget;
        } else {
            u -= rate;
            if (u < 0) { u = 0; }
            noPosition.x = rest.x + mag * u * sin(noShakeCount * 3.14159f * u);
            noPosition.y = (1-upRate) * noPosition.y + upRate * rest.y;

        }
        gno.BG0_setPanning(noPosition);
    
   	    System::paint();
    }
}
