/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include "monsters.h"
using namespace Sifteo;

static Metadata M = Metadata()
    .title("Monsters")
    .package("com.sifteo.extras.monsters", "1.0")
    .cubeRange(1);

void main()
{
    const CubeID cube(0);
    static VideoBuffer vid;

    vid.initMode(FB32);
    vid.attach(cube);

    float monster = 0.5f;
    while (1) {
        monster += cube.accel().x * 0.01f;
        const MonsterData *data = monsters[umod(monster, arraysize(monsters))];

        vid.colormap.set((RGB565*) &data->fb[512]);
        vid.fb32.set((uint16_t*) &data->fb[0]);

        System::paint();
    }
}
