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

    _SYS_vectors.cubeEvents.touch = onTouch;

    for (;;) {
        for (unsigned i = 0; i < NUM_CUBES; i++) {
            //uint8_t nb[4];
            _SYSCubeID id = cubes[i].id();
            _SYSNeighborState nb;
            _SYSCubeHWID hwid;
            _SYSAccelState accel;
            uint16_t battery;

            /*
             * Ugly in all sorts of ways...
             */

            _SYS_getAccel(id, &accel);
            _SYS_getNeighbors(id, &nb);
            _SYS_getRawBatteryV(id, &battery);
            _SYS_getCubeHWID(id, &hwid);

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
                             id,
                             hwid.bytes[0], hwid.bytes[1], hwid.bytes[2],
                             hwid.bytes[3], hwid.bytes[4], hwid.bytes[5],
                             nb.sides[0], nb.sides[1], nb.sides[2], nb.sides[3],
                             accel.x, accel.y,
                             battery);
                             
            drawSide(i, nb.sides[0] != CUBE_ID_UNDEFINED, 1,  0,  1, 0);  // Top
            drawSide(i, nb.sides[1] != CUBE_ID_UNDEFINED, 0,  1,  0, 1);  // Left
            drawSide(i, nb.sides[2] != CUBE_ID_UNDEFINED, 1,  15, 1, 0);  // Bottom
            drawSide(i, nb.sides[3] != CUBE_ID_UNDEFINED, 15, 1,  0, 1);  // Right
        }

        System::paint();
    }
}