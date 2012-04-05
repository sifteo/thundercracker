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
    .title("Hello World SDK Example")
    .icon(GameIcon);

static const unsigned gNumCubes = 1;
static VideoBuffer vid[CUBE_ALLOCATION];


static void onAccelChange(void *, unsigned cid)
{
    auto accel = CubeID(cid).accel();
    auto &draw = vid[cid].bg0;

    String<64> str;
    str << "Accel: " << Hex(accel.x + 0x80, 2) << " " << Hex(accel.y + 0x80, 2);
    LOG_STR(str);

    draw.text(Vec2(2,3), Font, str);
    draw.setPanning(accel.xy() / -2);
}

void main()
{
    Events::cubeAccelChange.set(onAccelChange);

    for (CubeID cube = 0; cube < gNumCubes; ++cube) {
        cube.enable();
        vid[cube].initMode(BG0);
        vid[cube].attach(cube);
        auto &draw = vid[cube].bg0;

        draw.erase(WhiteTile);
        draw.text(Vec2(2,1), Font, "Hello World!");
        draw.image(Vec2(1,10), Logo);

        onAccelChange(0, cube);
    } 

    while (1)
        System::paint();
}
