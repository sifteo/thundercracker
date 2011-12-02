#include <sifteo.h>
#include <stdlib.h>
#include "assets.gen.h"
#include "WordGame.h"
#include "EventID.h"
#include "EventData.h"

using namespace Sifteo;

static const char* sideNames[] =
{
  "top", "left", "bottom", "right"  
};

void onCubeEventTouch(_SYSCubeID cid)
{
    LOG(("cube event touch:\t%d\n", cid));
    GameStateMachine::sOnEvent(EventID_RequestNewAnagram, EventData());
}

void onCubeEventShake(_SYSCubeID cid)
{
    LOG(("cube event shake:\t%d\n", cid));
    GameStateMachine::sOnEvent(EventID_RequestNewAnagram, EventData());
}

void onCubeEventTilt(_SYSCubeID cid)
{
    LOG(("cube event tilt:\t%d\n", cid));
    GameStateMachine::sOnEvent(EventID_RequestNewAnagram, EventData());
}

void onNeighborEventAdd(_SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1)
{
    EventData data;
    GameStateMachine::sOnEvent(EventID_AddNeighbor, data);
    LOG(("neighbor add:\t%d/%s\t%d/%s\n", c0, sideNames[s0], c1, sideNames[s1]));
}

void onNeighborEventRemove(_SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1)
{
    EventData data;
    GameStateMachine::sOnEvent(EventID_RemoveNeighbor, data);
    LOG(("neighbor remove:\t%d/%s\t%d/%s\n", c0, sideNames[s0], c1, sideNames[s1]));
}

void accel(_SYSCubeID c)
{
    LOG(("accelerometer changed\n"));
}

void siftmain()
{
    LOG(("Hello, Word Play 2\n"));
    _SYS_vectors.cubeEvents.touch = onCubeEventTouch;
    _SYS_vectors.cubeEvents.shake = onCubeEventShake;
    _SYS_vectors.cubeEvents.tilt = onCubeEventTilt;
    _SYS_vectors.neighborEvents.add = onNeighborEventAdd;
    _SYS_vectors.neighborEvents.remove = onNeighborEventRemove;
    
    static Cube cubes[MAX_CUBES];

    //static CubeSim demos[] = { CubeSim(cubes[0]), CubeSim(cubes[1]) };
    
    // start loading assets
    for (unsigned i = 0; i < arraysize(cubes); i++)
    {
        cubes[i].enable(i);
        cubes[i].loadAssets(GameAssets);

        VidMode_BG0_ROM rom(cubes[i].vbuf);
        rom.init();
        
        rom.BG0_text(Vec2(1,1), "Loading...");
    }

    // wait for assets to finish loading
    for (;;)
    {
        bool done = true;

        for (unsigned i = 0; i < arraysize(cubes); i++)
        {
            VidMode_BG0_ROM rom(cubes[i].vbuf);
            rom.BG0_progressBar(Vec2(0,7), cubes[i].assetProgress(GameAssets, VidMode_BG0::LCD_width), 2);
            if (!cubes[i].assetDone(GameAssets))
            {
                done = false;
            }
        }

        System::paint();

        if (done)
        {
            break;
        }
    }

    /*
    for (unsigned i = 0; i < arraysize(demos); i++)
        demos[i].init();
    */

    // main loop
    static WordGame game(cubes);
    float lastTime = System::clock();
    while (1)
    {
        float now = System::clock();
        float dt = now - lastTime;
        lastTime = now;

        game.update(dt);
        game.onEvent(EventID_Paint, EventData()); // TODO decouple
        
        System::paint();
    }
}

/*  The __cxa_pure_virtual function is an error handler that is invoked when a
    pure virtual function is called. If you are writing a C++ application that
    has pure virtual functions you must supply your own __cxa_pure_virtual
    error handler function. For example:
*/
extern "C" void __cxa_pure_virtual() { while (1); }
