/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(MainAssets);

static Metadata M = Metadata()
    .title("Drawing test")
    .cubeRange(1);

void main()
{
    static VideoBuffer vid;
    const CubeID cube(0);

    vid.initMode(BG0_SPR_BG1);
    vid.bg0.erase(Background);
    vid.attach(cube);

    unsigned animFrame = 0;
    volatile uint32_t x = 0;

    while (1) {
        vid.bg1.maskedImage(Animation, Transparent, animFrame);
        if (++animFrame == Animation.numFrames()) animFrame = 0;

        vid.bg0.setPanning(vid.bg0.getPanning() + vec(1,0));
        System::paint();
        System::finish();

        SCRIPT(LUA,
            rt = Runtime();
            fe = Frontend();
            print("Hello World!");
        );

        SCRIPT_FMT(LUA,
            "fe:postMessage('Hello! animFrame=%d');",
            animFrame);

        SCRIPT_FMT(LUA,
            "rt:poke(%p, 12345);",
            &x);

        LOG_INT(x);
    }
}