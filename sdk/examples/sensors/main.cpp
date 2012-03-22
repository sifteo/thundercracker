/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>

using namespace Sifteo;

#ifndef NUM_CUBES
#  define NUM_CUBES 3
#endif

static Cube cubes[NUM_CUBES];

struct counts_t {
    unsigned touch, neighborAdd, neighborRemove;
};
        
void drawSide(int cube, bool filled, int x, int y, int dx, int dy)
{
    for (unsigned i = 0; i < 14; i++) {
        VidMode_BG0_ROM vid(cubes[cube].vbuf);
        vid.BG0_putTile(Vec2(x,y), filled ? 0x9ff : 0);
        x += dx;
        y += dy;
    }
}

static void onTouch(counts_t *counts, _SYSCubeID cid)
{    
    counts[cid].touch++;
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

void main()
{
    static counts_t counts[NUM_CUBES];
    
    for (unsigned i = 0; i < NUM_CUBES; i++) {
        cubes[i].enable(i);
        VidMode_BG0_ROM(cubes[i].vbuf).init();
    }

    _SYS_setVector(_SYS_CUBE_TOUCH, (void*) onTouch, (void*) counts);
    _SYS_setVector(_SYS_NEIGHBOR_ADD, (void*) onNeighborAdd, (void*) counts);
    _SYS_setVector(_SYS_NEIGHBOR_REMOVE, (void*) onNeighborRemove, (void*) counts);

    for (;;) {
        for (unsigned i = 0; i < NUM_CUBES; i++) {
            Cube &cube = cubes[i]; 
            VidMode_BG0_ROM vid(cube.vbuf);
            String<128> str;

            uint64_t hwid = cube.hardwareID();
            str << "I am cube #" << cube.id() << "\n\n";
            str << "hwid " << Hex(hwid >> 32) << "\n     " << Hex(hwid) << "\n\n";

            _SYSNeighborState nb;
            _SYS_getNeighbors(cube.id(), &nb);
            str << "nb "
                << Hex(nb.sides[0], 2) << " "
                << Hex(nb.sides[1], 2) << " "
                << Hex(nb.sides[2], 2) << " "
                << Hex(nb.sides[3], 2) << "\n";
            
            str << "   +" << counts[cube.id()].neighborAdd
                << ", -" << counts[cube.id()].neighborRemove
                << "\n\n";

            str << "bat:   " << Hex(_SYS_getRawBatteryV(cube.id()), 4) << "\n";
            str << "touch: " << counts[cube.id()].touch << "\n";

            _SYSAccelState accel = _SYS_getAccel(i);
            str << "acc: "
                << Fixed(accel.x, 3)
                << Fixed(accel.y, 3)
                << Fixed(accel.z, 3);
                
            vid.BG0_text(Vec2(1,2), str);

            drawSide(i, nb.sides[0] != CUBE_ID_UNDEFINED, 1,  0,  1, 0);  // Top
            drawSide(i, nb.sides[1] != CUBE_ID_UNDEFINED, 0,  1,  0, 1);  // Left
            drawSide(i, nb.sides[2] != CUBE_ID_UNDEFINED, 1,  15, 1, 0);  // Bottom
            drawSide(i, nb.sides[3] != CUBE_ID_UNDEFINED, 15, 1,  0, 1);  // Right
        }

        System::paint();
    }
}
