/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(GameAssets);

static Metadata M = Metadata()
    .title("Nyan~")
    .cubeRange(1);

void main()
{
    const CubeID cube(0);
    static VideoBuffer vid;
    static AudioChannel channel;

    channel.play(Nyan, channel.REPEAT);

    cube.enable();
    vid.initMode(BG0);
    vid.attach(cube);

    while (1) {
        unsigned frame = SystemTime::now().cycleFrame(0.5, Cat.numFrames());
        vid.bg0.image(vec(0,0), Cat, frame);
        System::paint();
    }
}
