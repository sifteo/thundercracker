#include <sifteo.h>
//#include "assets.gen.h"

using namespace Sifteo;

#ifndef NUM_CUBES
#  define NUM_CUBES 6
#endif

static Cube cubes[] = { Cube(0), Cube(1), Cube(2), Cube(3), Cube(4), Cube(5) };
static VidMode_BG0 vid[] = { VidMode_BG0(cubes[0].vbuf), VidMode_BG0(cubes[1].vbuf), VidMode_BG0(cubes[2].vbuf),
                             VidMode_BG0(cubes[3].vbuf), VidMode_BG0(cubes[4].vbuf), VidMode_BG0(cubes[5].vbuf) };



static void onCubeFound(void *context, _SYSCubeID cid)
{
    LOG(("++ onCubeFound: %u\n", cid));
}

static void onCubeLost(void *context, _SYSCubeID cid)
{
    LOG(("-- onCubeLost: %u\n", cid));
}

static void init()
{
    LOG(("Initializing...\n"));
    
    // NOTE: Enabling cubes is required before accel change events are delivered.
    for (unsigned i = 0; i < NUM_CUBES; i++) {
        cubes[i].enable();
        
        VidMode_BG0_ROM rom(cubes[i].vbuf);
        rom.init();
        rom.BG0_text(Vec2(1,1), "Loading...");
    }
    
    for (unsigned i = 0; i < NUM_CUBES; i++) {
        vid[i].init();
    }
}

void main()
{
    LOG(("main\n"));
    
    System::solicitCubes(3, 6);

    init();
    
    _SYS_setVector(_SYS_CUBE_FOUND, (void*) onCubeFound, NULL);
    _SYS_setVector(_SYS_CUBE_LOST, (void*) onCubeLost, NULL);
    
    while (1) {
        //float t = System::clock();
        //LOG(("clock: %f\n", t));
        
        System::paint();
    }
}
