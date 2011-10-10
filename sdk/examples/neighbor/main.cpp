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

void fillSide(int cube, int x, int y, int dx, int dy)
{
    static const uint16_t pattern[] = { 0x1145, 0x1148, 0x1144 };
    int patternIndex = 0;

    for (unsigned i = 0; i < 16; i++) {
        vid[cube].BG0_putTile(Vec2(x,y), pattern[patternIndex]);
        x += dx;
        y += dy;
        if (++patternIndex == arraysize(pattern))
            patternIndex = 0;
    }
}

void siftmain()
{
    for (unsigned i = 0; i < NUM_CUBES; i++) {
        cubes[i].enable();
        vid[i].init();
    }

    for (;;) {
        for (unsigned i = 0; i < NUM_CUBES; i++) {
            uint8_t buf[4];
            _SYSAccelState accel;

            /*
             * Ugly in all sorts of ways...
             */

            _SYS_getAccel(i, &accel);
            _SYS_getRawNeighbors(i, buf);

            vid[i].init();
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

            if (buf[0] & 0x80) fillSide(i, 0,  0,  1, 0);  // Top
            if (buf[1] & 0x80) fillSide(i, 15, 0,  0, 1);  // Right
            if (buf[2] & 0x80) fillSide(i, 0,  15, 1, 0);  // Bottom
            if (buf[3] & 0x80) fillSide(i, 0,  0,  0, 1);  // Left
        }

        System::paint();
    }
}
