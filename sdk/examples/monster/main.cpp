/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "monsters.h"
using namespace Sifteo;

static Metadata M = Metadata()
    .title("Monsters SDK Example")
    .cubeRange(1);

static const CubeID cube(0);
static VideoBuffer vid;

static void showMonster(const MonsterData *m)
{
    vid.colormap.set((RGB565*) &m->fb[512]);
    vid.fb32.set((uint16_t*) &m->fb[0]);
}

void main()
{
    vid.initMode(FB32);
    vid.attach(cube);

    float monster = 0.5f;
    while (1) {
        monster += cube.accel().x * 0.01f;
        showMonster(monsters[umod(monster, arraysize(monsters))]);
        System::paint();
    }
}