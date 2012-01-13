/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>

using namespace Sifteo;

#define NUM_CUBES 3

static Cube cubes[NUM_CUBES];

static struct {
    unsigned touch, neighborAdd, neighborRemove;
} counts[_SYS_NUM_CUBE_SLOTS];
    
    
void drawSide(int cube, bool filled, int x, int y, int dx, int dy)
{
    for (unsigned i = 0; i < 14; i++) {
        VidMode_BG0_ROM vid(cubes[cube].vbuf);
        vid.BG0_putTile(Vec2(x,y), filled ? 0x9ff : 0);
        x += dx;
        y += dy;
    }
}

static void onTouch(_SYSCubeID cid)
{    
    counts[cid].touch++;
}

static void onNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
{
    counts[c0].neighborAdd++;
    counts[c1].neighborAdd++;
}

static void onNeighborRemove(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
{
    counts[c0].neighborRemove++;
    counts[c1].neighborRemove++;
}

void siftmain()
{
    for (unsigned i = 0; i < NUM_CUBES; i++) {
        cubes[i].enable(i);

        VidMode_BG0_ROM vid(cubes[i].vbuf);
        vid.init();
    }

    _SYS_vectors.cubeEvents.touch = onTouch;
    _SYS_vectors.neighborEvents.add = onNeighborAdd;
    _SYS_vectors.neighborEvents.remove = onNeighborRemove;

    for (;;) {
        for (unsigned i = 0; i < NUM_CUBES; i++) {
            _SYSCubeID id = cubes[i].id();
            VidMode_BG0_ROM vid(cubes[i].vbuf);

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

            vid.BG0_textf(Vec2(1,2),
                          "I am cube #%d\n"
                          "\n"
                          "id %02x%02x%02x%02x%02x%02x\n"
                          "\n"
                          "nb %02x %02x %02x %02x\n"
                          "   +%d, -%d\n"
                          "\n"
                          "bat:   %04x\n"
                          "touch: %d\n"
                          "\n"
                          "acc: %3d %3d\n",
                          id,
                          hwid.bytes[0], hwid.bytes[1], hwid.bytes[2],
                          hwid.bytes[3], hwid.bytes[4], hwid.bytes[5],
                          nb.sides[0], nb.sides[1], nb.sides[2], nb.sides[3],
                          counts[id].neighborAdd, counts[id].neighborRemove,
                          battery,
                          counts[id].touch,
                          accel.x, accel.y);
                             
            drawSide(i, nb.sides[0] != CUBE_ID_UNDEFINED, 1,  0,  1, 0);  // Top
            drawSide(i, nb.sides[1] != CUBE_ID_UNDEFINED, 0,  1,  0, 1);  // Left
            drawSide(i, nb.sides[2] != CUBE_ID_UNDEFINED, 1,  15, 1, 0);  // Bottom
            drawSide(i, nb.sides[3] != CUBE_ID_UNDEFINED, 15, 1,  0, 1);  // Right
        }

        System::paint();
    }
}