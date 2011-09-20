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
    for (unsigned i = 0; i < sizeof *m / 2; i++)
	cube.vbuf.poke(i, ((uint16_t *)m->fb)[i]);

    cube.vbuf.unlock();
}

void siftmain()
{
    unsigned fpMonster = 0;
    const MonsterData *currentMonster = NULL;

    cube.vbuf.init();
    cube.vbuf.sys.vram.mode = _SYS_VM_FB32;
    cube.vbuf.sys.vram.flags = _SYS_VF_CONTINUOUS;
    cube.enable();

    while (1) {
	_SYSAccelState state;
	_SYS_getAccel(cube.id(), &state);
	fpMonster += state.x;

	const MonsterData *m = monsters[ (fpMonster >> 14) % arraysize(monsters) ];
	if (m != currentMonster) {
	    showMonster(m);
	    currentMonster = m;
	}

	System::paint();
    }
}
