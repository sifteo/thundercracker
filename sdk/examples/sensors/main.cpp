/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
using namespace Sifteo;

static const unsigned gNumCubes = 3;

struct counts_t {
    unsigned touch, shake, neighborAdd, neighborRemove;
};

#if 0

static void onTouch(counts_t *counts, _SYSCubeID cid)
{    
    counts[cid].touch++;
}

static void onShake(counts_t *counts, _SYSCubeID cid)
{    
    counts[cid].shake++;
}

static void onNeighborAdd(counts_t *counts,
    Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
{
    LOG(("Neighbor Add: %d:%d - %d:%d\n", c0, s0, c1, s1));
    counts[c0].neighborAdd++;
    counts[c1].neighborAdd++;
}

static void onNeighborRemove(counts_t *counts,
    Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
{
    LOG(("Neighbor Remove: %d:%d - %d:%d\n", c0, s0, c1, s1));
    counts[c0].neighborRemove++;
    counts[c1].neighborRemove++;
}
#endif


void main()
{
    static counts_t counts[CUBE_ALLOCATION];
    static VideoBuffer vid[CUBE_ALLOCATION];

    for (CubeID cube = 0; cube < gNumCubes; ++cube) {
        cube.enable();
        vid[cube].initMode(BG0_ROM);
        vid[cube].attach(cube);
    }

    while (1) {
        for (CubeID cube = 0; cube < gNumCubes; ++cube) {
            auto &draw = vid[cube].bg0rom;
            String<192> str;

            /*
             * Textual dump of current sensor state
             */

            uint64_t hwid = cube.getHWID();
            str << "I am cube #" << cube << "\n";
            str << "hwid " << Hex(hwid >> 32) << "\n     " << Hex(hwid) << "\n\n";

            Neighborhood nb(cube);
            str << "nb "
                << Hex(nb.neighborAt(TOP), 2) << " "
                << Hex(nb.neighborAt(LEFT), 2) << " "
                << Hex(nb.neighborAt(BOTTOM), 2) << " "
                << Hex(nb.neighborAt(RIGHT), 2) << "\n";

            str << "   +" << counts[cube].neighborAdd
                << ", -" << counts[cube].neighborRemove
                << "\n\n";

            str << "bat:   " << Hex(cube.getBattery(), 4) << "\n";
            str << "touch: " << cube.isTouching() <<
                " (" << counts[cube].touch << ")\n";

            auto accel = cube.getAccel();
            str << "acc: "
                << Fixed(accel.x, 3)
                << Fixed(accel.y, 3)
                << Fixed(accel.z, 3) << "\n";

            auto tilt = cube.getTilt();
            str << "tilt:"
                << Fixed(tilt.x, 3)
                << Fixed(tilt.y, 3) << "\n";

            str << "shake: " << counts[cube].shake;

            draw.text(Vec2(1,2), str);

            /*
             * Neighboring indicator bars
             */

            unsigned nbColor = draw.ORANGE;

            draw.fill(nbColor | (nb.hasNeighborAt(TOP) ? draw.SOLID_FG : draw.SOLID_BG),
                Vec2(1, 0), Vec2(14, 1));

            draw.fill(nbColor | (nb.hasNeighborAt(LEFT) ? draw.SOLID_FG : draw.SOLID_BG),
                Vec2(0, 1), Vec2(1, 14));

            draw.fill(nbColor | (nb.hasNeighborAt(BOTTOM) ? draw.SOLID_FG : draw.SOLID_BG),
                Vec2(1, 15), Vec2(14, 1));

            draw.fill(nbColor | (nb.hasNeighborAt(RIGHT) ? draw.SOLID_FG : draw.SOLID_BG),
                Vec2(15, 1), Vec2(1, 14));
        }

        System::paint();
    }

#if 0
    _SYS_setVector(_SYS_CUBE_TOUCH, (void*) onTouch, (void*) counts);
    _SYS_setVector(_SYS_CUBE_SHAKE, (void*) onShake, (void*) counts);
    _SYS_setVector(_SYS_NEIGHBOR_ADD, (void*) onNeighborAdd, (void*) counts);
    _SYS_setVector(_SYS_NEIGHBOR_REMOVE, (void*) onNeighborRemove, (void*) counts);
#endif
}
