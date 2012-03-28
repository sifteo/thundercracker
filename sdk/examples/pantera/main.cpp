#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

#define NUM_CUBES 3

static Cube cubes[NUM_CUBES];

void main() {
    for (unsigned i=0; i<NUM_CUBES; i++) {
        LOG(("ENABLE %d\n", i));
        cubes[i].enable(i);
        cubes[i].loadAssets(GameAssets);
        VidMode_BG0_ROM rom(cubes[i].vbuf);
        rom.init();
        rom.BG0_text(Vec2(1,1), "Loading...");
    }
    bool done = false;
    while (!done) {
        done = true;
        for (unsigned i = 0; i < NUM_CUBES; i++) {
            VidMode_BG0_ROM rom(cubes[i].vbuf);
            rom.BG0_progressBar(Vec2(0,7), cubes[i].assetProgress(GameAssets, VidMode_BG0::LCD_width), 2);
            if (!cubes[i].assetDone(GameAssets)) {
                done = false;
            }
        }
        System::paint();
    }

    System::paintSync();
    for (unsigned i=0; i<NUM_CUBES; i++) {
        VidMode_BG0 g(cubes[i].vbuf);
        g.init();
        g.BG0_drawAsset(Vec2(0,0), Background);
        g.BG0_setPanning(Vec2(8,8));
    }
    System::paintSync();
    for (unsigned i=0; i<NUM_CUBES; ++i) { cubes[i].vbuf.touch(); }
    System::paintSync();
    while (1) {
        VidMode_BG0 g(cubes[0].vbuf);
        Int2 accel = cubes[0].physicalAccel();
        g.BG0_setPanning(Vec2(8.f,8.f) - accel/2.f);
        cubes[0].vbuf.touch();
    	System::paint();
    }
}
