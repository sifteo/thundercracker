/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>

using namespace Sifteo;

#define NUM_CUBES 2
static Cube cubes[] = { Cube(0), Cube(1) };
static VidMode_BG0_ROM vid[] = { VidMode_BG0_ROM(cubes[0].vbuf), VidMode_BG0_ROM(cubes[1].vbuf) };


void drawSide(int cube, bool filled, int x, int y, int dx, int dy)
{
    for (unsigned i = 0; i < 14; i++) {
        vid[cube].BG0_putTile(Vec2(x,y), filled ? 0x9ff : 0);
        x += dx;
        y += dy;
    }
}

static void onTouch(_SYSCubeID cid)
{
    LOG(("Cube %d, touch detected\n", cid));
}

void siftmain()
{
    for (unsigned i = 0; i < NUM_CUBES; i++) {
        cubes[i].enable();
        vid[i].init();
    }

    _SYS_vectors.touch = onTouch;

    for (;;) {
        for (unsigned i = 0; i < NUM_CUBES; i++) {
            uint8_t buf[4];
            _SYSAccelState accel;

            /*
             * Ugly in all sorts of ways...
             */

            _SYS_getAccel(i, &accel);
            _SYS_getRawNeighbors(i, buf);

            vid[i].BG0_textf(Vec2(1,2),
                             "Neighbor Test!\n"
                             "\n"
                             "I am cube #%d\n"
                             "\n"
                             "Raw data:\n"
                             " %02x %02x %02x %02x\n"
                             "\n"
                             "Accel:\n"
                             " %02x %02x\n",
                             i, buf[0], buf[1], buf[2], buf[3],
                             accel.x + 0x80,
                             accel.y + 0x80);

            drawSide(i, buf[0] >> 7, 1,  0,  1, 0);  // Top
            drawSide(i, buf[1] >> 7, 0,  1,  0, 1);  // Left
            drawSide(i, buf[2] >> 7, 1,  15, 1, 0);  // Bottom
            drawSide(i, buf[3] >> 7, 15, 1,  0, 1);  // Right
        }

        System::paint();
    }
}
