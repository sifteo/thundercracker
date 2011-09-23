/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
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

static void onAccelChange(_SYSCubeID cid)
{
    _SYSAccelState state;
    _SYS_getAccel(cid, &state);

    static unsigned i = 0;
    const MonsterData *m = monsters[i];
    i = (i + 1) % arraysize(monsters);

    showMonster(m);
}

void siftmain()
{
    cube.vbuf.init();
    memset(cube.vbuf.sys.vram.words, 0, sizeof cube.vbuf.sys.vram.words);
    cube.vbuf.sys.vram.mode = _SYS_VM_FB32;
    cube.vbuf.sys.vram.flags = _SYS_VF_CONTINUOUS;
    cube.vbuf.unlock();

    _SYS_vectors.accelChange = onAccelChange;
    cube.enable();

    memcpy(cube.vbuf.sys.vram.fb, &marlock_mavenfarm, sizeof(struct MonsterData));


    while (1) {
	System::paint();
    }
}
