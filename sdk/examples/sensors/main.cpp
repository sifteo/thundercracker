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

void siftmain()
{
    static counts_t counts[NUM_CUBES];
    
    for (unsigned i = 0; i < NUM_CUBES; i++) {
        cubes[i].enable(i);

        VidMode_BG0_ROM vid(cubes[i].vbuf);
        vid.init();
    }

    _SYS_setVector(_SYS_CUBE_TOUCH, (void*) onTouch, (void*) counts);
    _SYS_setVector(_SYS_NEIGHBOR_ADD, (void*) onNeighborAdd, (void*) counts);
    _SYS_setVector(_SYS_NEIGHBOR_REMOVE, (void*) onNeighborRemove, (void*) counts);

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

            accel = _SYS_getAccel(id);
            _SYS_getNeighbors(id, &nb);
            battery = _SYS_getRawBatteryV(id);
            _SYS_getCubeHWID(id, &hwid);

            static String<128> str;
            str.clear();

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