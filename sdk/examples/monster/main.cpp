/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include <stdio.h>
#include <string.h>
#include "monsters.h"

using namespace Sifteo;

static Cube cube(0);

static void showMonster(const MonsterData *m)
{
    // XXX: Waiting for a real compare-and-copy syscall
    for (unsigned i = 0; i < 256; i++)
        cube.vbuf.poke(i, ((uint16_t *)m->fb)[i]);

    for (unsigned i = 0; i < 16; i++)
        cube.vbuf.poke(i + 384, ((uint16_t *)m->fb)[i + 256]);
}

void siftmain()
{
    int fpMonster = 0;
    const int shift = 7;
    const int fpMax = arraysize(monsters) << shift;

    cube.vbuf.init();
    cube.vbuf.sys.vram.mode = _SYS_VM_FB32;
    cube.vbuf.sys.vram.num_lines = 128;
    cube.enable();

    while (1) {
        _SYSAccelState state;
        _SYS_getAccel(cube.id(), &state);

        fpMonster += state.x;

        while (fpMonster < 0) fpMonster += fpMax;
        while (fpMonster > fpMax) fpMonster -= fpMax;

        showMonster(monsters[fpMonster >> shift]);
        System::paint();
    }
}


//USED FOR CES ONLY
void selectormain()
{
    return;
}
