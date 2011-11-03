/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
1 */

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

    _SYS_vectors.table[_SYS_EVENT_TOUCH] = onTouch;

    for (;;) {
        for (unsigned i = 0; i < NUM_CUBES; i++) {
            uint8_t nb[4];
            _SYSCubeHWID hwid;
            _SYSAccelState accel;
            uint16_t battery;

            /*
             * Ugly in all sorts of ways...
             */

            _SYS_getAccel(i, &accel);
            _SYS_getRawNeighbors(i, nb);
            _SYS_getRawBatteryV(i, &battery);
            _SYS_getCubeHWID(i, &hwid);

            vid[i].BG0_textf(Vec2(1,2),
                             "I am cube #%d\n"
                             "\n"
                             "Hardware ID:\n"
                             " %02x%02x%02x%02x%02x%02x\n"
                             "\n"
                             "Raw nb/touch:\n"
                             " %02x %02x %02x %02x\n"
                             "\n"
                             "Raw accel:\n"
                             " %4d %4d\n"
                             "\n"
                             "Raw battery:\n"
                             " %04x\n",
                             i,
                             hwid.bytes[0], hwid.bytes[1], hwid.bytes[2],
                             hwid.bytes[3], hwid.bytes[4], hwid.bytes[5],
                             nb[0], nb[1], nb[2], nb[3],
                             accel.x, accel.y,
                             battery);
                             
            drawSide(i, nb[0] >> 7, 1,  0,  1, 0);  // Top
            drawSide(i, nb[1] >> 7, 0,  1,  0, 1);  // Left
            drawSide(i, nb[2] >> 7, 1,  15, 1, 0);  // Bottom
            drawSide(i, nb[3] >> 7, 15, 1,  0, 1);  // Right
        }

        System::paint();
    }
}
