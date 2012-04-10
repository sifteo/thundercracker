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
    TileBuffer<5,10> buf(cube);

    buf.text(vec(0,0), Font, "Hello");

    vid.initMode(BG0);
    vid.bg0.erase(WhiteTile);
    vid.bg0.image(vec(2,1), buf);
    vid.bg0.image(vec(7,8), buf);
    vid.bg0.image(vec(3,13), buf);
    vid.attach(cube);

    System::paint();
    System::finish();
}