#include <sifteo.h>
#include "assets.gen.h"
#include "assets.gen.h"
#include "WordGame.h"
#include "EventID.h"
#include "EventData.h"

using namespace Sifteo;

static const char* sideNames[] =
{
  "top", "left", "bottom", "right"  
};

void onCubeEventTouch(void *context, _SYSCubeID cid)
{
    DEBUG_LOG(("cube event touch:\t%d\n", cid));
/* TODO Touch    EventData data;
    data.mInput.mCubeID = cid;
    WordGame::onEvent(EventID_Touch, data);
    */

#ifdef DEBUG
    DEBUG_LOG(("cube event touch->shake, ID:\t%d\n", cid));
    EventData data;
    data.mInput.mCubeID = cid;
    WordGame::onEvent(EventID_Shake, data);
#endif
}

void onCubeEventShake(void *context, _SYSCubeID cid)
{
    DEBUG_LOG(("cube event shake, ID:\t%d\n", cid));
    EventData data;
    data.mInput.mCubeID = cid;
    WordGame::onEvent(EventID_Shake, data);
}

void onCubeEventTilt(void *context, _SYSCubeID cid)
{
    DEBUG_LOG(("cube event tilt:\t%d\n", cid));
    EventData data;
    data.mInput.mCubeID = cid;
    WordGame::onEvent(EventID_Tilt, data);
}

void onNeighborEventAdd(void *context,
    _SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1)
{
    EventData data;
    WordGame::onEvent(EventID_AddNeighbor, data);
    LOG(("neighbor add:\t%d/%s\t%d/%s\n", c0, sideNames[s0], c1, sideNames[s1]));
}

void onNeighborEventRemove(void *context,
    _SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1)
{
    EventData data;
    WordGame::onEvent(EventID_RemoveNeighbor, data);
    LOG(("neighbor remove:\t%d/%s\t%d/%s\n", c0, sideNames[s0], c1, sideNames[s1]));
}

void accel(_SYSCubeID c)
{
    DEBUG_LOG(("accelerometer changed\n"));
}

void main()
{
    DEBUG_LOG(("Hello, Word Play 2\n"));

    _SYS_setVector(_SYS_CUBE_TOUCH, (void*) onCubeEventTouch, NULL);
    _SYS_setVector(_SYS_CUBE_SHAKE, (void*) onCubeEventShake, NULL);
    _SYS_setVector(_SYS_CUBE_TILT, (void*) onCubeEventTilt, NULL);
    _SYS_setVector(_SYS_NEIGHBOR_ADD, (void*) onNeighborEventAdd, NULL);
    _SYS_setVector(_SYS_NEIGHBOR_REMOVE, (void*) onNeighborEventRemove, NULL);

    static Cube cubes[NUM_CUBES]; // must be static!

    for (unsigned i = 0; i < arraysize(cubes); i++)
    {
        cubes[i].enable(i + CUBE_ID_BASE);
    }

#ifndef DEBUGz
    if (LOAD_ASSETS)
    {
        // start loading assets
        for (unsigned i = 0; i < arraysize(cubes); i++)
        {
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
                rom.BG0_progressBar(Vec2(0,7),
                                    cubes[i].assetProgress(GameAssets,
                                                           VidMode_BG0_SPR_BG1::LCD_width),
                                    2);

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
    }
#endif // ifndef DEBUG

    // main loop
    WordGame game(cubes); // must not be static!

    TimeStep ts;
    ts.next();
    SystemTime lastPaint = ts.end();

    while (1)
    {
        ts.next();
        game.update(ts.delta());

        // decouple paint frequency from update frequency
        if (ts.end() - lastPaint >= 1.f/25.f)
        {
            game.onEvent(EventID_Paint, EventData());
            lastPaint = ts.end();
        }

        if (true || game.needsPaintSync()) // TODO can't seem to fix BG1 weirdness w/o this
        {
            game.paintSync();
        }
        else
        {
            System::paint(); // (will do nothing if screens haven't changed
        }
    }
}

/*  The __cxa_pure_virtual function is an error handler that is invoked when a
    pure virtual function is called. If you are writing a C++ application that
    has pure virtual functions you must supply your own __cxa_pure_virtual
    error handler function. For example:
*/
extern "C" void __cxa_pure_virtual() { while (1); }



void assertWrapper(bool testResult)
{
#ifdef SIFTEO_SIMULATOR
    if (!testResult)
    {
        assert(testResult);
    }
#endif
}
