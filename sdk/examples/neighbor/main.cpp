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

void paintSide(int cube, bool fill, int x, int y, int dx, int dy)
{
    for (unsigned i = 0; i < 14; i++) {
        vid[cube].BG0_putTile(Vec2(x,y), fill ? 0x5ff : 0x000);
        x += dx;
        y += dy;
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

            _SYS_getRawNeighbors(i, buf);

            vid[i].BG0_textf(Vec2(1,2),
                             "Neighbor Test!\n"
                             "\n"
                             "%02x %02x %02x %02x",
                             buf[0], buf[1], buf[2], buf[3]);

            paintSide(i, buf[0] >> 7, 1, 0, 1, 0);   // Top
            paintSide(i, buf[1] >> 7, 15, 1, 0, 1);  // Right
            paintSide(i, buf[2] >> 7, 1, 15, 1, 0);  // Bottom
            paintSide(i, buf[3] >> 7, 0, 1, 0, 1);   // Left
        }

        System::paint();
    }
}
