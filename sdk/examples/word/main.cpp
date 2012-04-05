#include <sifteo.h>
#include "assets.gen.h"
#include "assets.gen.h"
#include "WordGame.h"
#include "EventID.h"
#include "EventData.h"
#include "menu.h"

using namespace Sifteo;

static const char* sideNames[] =
{
  "top", "left", "bottom", "right"  
};

static struct MenuItem gItems[] =
{ {&IconContinue, &LabelContinue}, {&IconCity0, &LabelCity0}, {NULL, NULL} };

static struct MenuAssets gAssets =
{&BgTile, &Footer, &LabelEmpty, {&Tip0, &Tip1, &Tip2, NULL}};

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

void siftmain()
{
    DEBUG_LOG(("Hello, Word Caravan\n"));

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
    if (0 && LOAD_ASSETS)
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

    // sync up
    System::paintSync();
    for(Cube* p=cubes; p!=cubes+NUM_CUBES; ++p) { p->vbuf.touch(); }
    System::paintSync();
    for(Cube* p=cubes; p!=cubes+NUM_CUBES; ++p) { p->vbuf.touch(); }
    System::paintSync();
    Menu m(&cubes[0], &gAssets, gItems);

        struct MenuEvent e;
        while(1) {
            while(m.pollEvent(&e)) {
                switch(e.type) {
                    case MENU_ITEM_PRESS:
                        // Game Buddy is not clickable, so don't do anything on press
                        if (e.item >= 3) {
                            m.preventDefault();
                        }
                        if (e.item == 4) {
                            static unsigned randomIcon = 0;
                            randomIcon = (randomIcon + 1) % 4;
                            m.replaceIcon(e.item, gItems[randomIcon].icon);
                        }
                        break;
                    case MENU_EXIT:
                        // this is not possible when pollEvent is used as the condition to the while loop.
                        // NOTE: this event should never have its default handler skipped.
                        ASSERT(false);
                        break;

                    case MENU_NEIGHBOR_ADD:
                        LOG(("found cube %d on side %d of menu (neighbor's %d side)\n",
                             e.neighbor.neighbor, e.neighbor.masterSide, e.neighbor.neighborSide));
                        break;
                    case MENU_NEIGHBOR_REMOVE:
                        LOG(("lost cube %d on side %d of menu (neighbor's %d side)\n",
                             e.neighbor.neighbor, e.neighbor.masterSide, e.neighbor.neighborSide));
                        break;

                    case MENU_ITEM_ARRIVE:
                        LOG(("arriving at menu item %d\n", e.item));
                        break;
                    case MENU_ITEM_DEPART:
                        LOG(("departing from menu item %d\n", e.item));
                        break;
                    case MENU_PREPAINT:
                        // if you are drawing/animating the other cubes, do your work here
                    // do your implementation-specific drawing here
                        // NOTE: this event should never have its default handler skipped.
                        break;
                    case MENU_UNEVENTFUL:
                        // this should never happen. if it does, it can/should be ignored.
                        ASSERT(false);
                        break;
                }
            }
            ASSERT(e.type == MENU_EXIT);
            LOG(("Selected Game: %d\n", e.item));
        }

    // main loop
    WordGame game(cubes); // must not be static!
    // TODO use clockNS, to avoid precision bugs with long play sessions
    float lastTime = System::clock();
    float lastPaint = System::clock();
    while (1)
    {
        float now = System::clock();
        float dt = now - lastTime;
        lastTime = now;

        game.update(dt);

        // decouple paint frequency from update frequency
        if (now - lastPaint >= 1.f/25.f)
        {
            game.onEvent(EventID_Paint, EventData());
            lastPaint = now;
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
