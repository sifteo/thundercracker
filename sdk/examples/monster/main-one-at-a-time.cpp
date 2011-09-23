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
    cube.vbuf.init();

    // XXX: Waiting for a real compare-and-copy syscall
    for (unsigned i = 0; i < sizeof *m / 2; i++)
	cube.vbuf.poke(i, ((uint16_t *)m->fb)[i]);

    cube.vbuf.sys.vram.mode = _SYS_VM_FB32;
    cube.vbuf.sys.vram.flags = _SYS_VF_CONTINUOUS;

    cube.vbuf.unlock();
}

void siftmain()
{
    cube.enable();

    for (unsigned m = 0; m < arraysize(monsters); m++) {
	printf("Showing monster %d\n", m);
	showMonster(monsters[m]);

	for (unsigned j = 0; j < 2000; j++)
	    System::paint();
    }
}
