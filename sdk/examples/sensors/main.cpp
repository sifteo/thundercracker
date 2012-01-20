/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>

using namespace Sifteo;

#ifndef NUM_CUBES
#  define NUM_CUBES 1
#endif

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

            String<128> str;

            str << "I am cube #" << id << "\n";
            
            str << Hex(hwid.bytes[0], 2)
                << Hex(hwid.bytes[1], 2)
                << Hex(hwid.bytes[2], 2)
                << Hex(hwid.bytes[3], 2)
                << Hex(hwid.bytes[4], 2)
                << Hex(hwid.bytes[5], 2)
                << "\n\n";
                
            str << "nb "
                << Hex(nb.sides[0], 2) << " "
                << Hex(nb.sides[1], 2) << " "
                << Hex(nb.sides[2], 2) << " "
                << Hex(nb.sides[3], 2) << "\n";
            
            str << "   +" << counts[id].neighborAdd
                << ", -" << counts[id].neighborRemove
                << "\n\n";
            
            str << "bat:   " << Hex(battery, 4) << "\n";
            str << "touch: " << counts[id].touch << "\n\n";
            
            str << "acc: " << Fixed(accel.x, 3) << " "
                << Fixed(accel.y, 3) << "\n";

            vid.BG0_text(Vec2(1,2), str);

            drawSide(i, nb.sides[0] != CUBE_ID_UNDEFINED, 1,  0,  1, 0);  // Top
            drawSide(i, nb.sides[1] != CUBE_ID_UNDEFINED, 0,  1,  0, 1);  // Left
            drawSide(i, nb.sides[2] != CUBE_ID_UNDEFINED, 1,  15, 1, 0);  // Bottom
            drawSide(i, nb.sides[3] != CUBE_ID_UNDEFINED, 15, 1,  0, 1);  // Right
        }

        System::paint();
    }
}