/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include <sifteo/menu.h>
#include "assets.gen.h"
using namespace Sifteo;

// Constants
#define NUM_CUBES           3
#define LOAD_ASSETS         1

#define NUM_ITEMS           arraysize(gItems);
#define NUM_TIPS            3

// Static Globals
static VideoBuffer gVideo[NUM_CUBES];
static struct MenuItem gItems[] = { {&IconChroma, &LabelChroma}, {&IconSandwich, &LabelSandwich}, {&IconPeano, &LabelPeano}, {&IconBuddy, &LabelBuddy}, {&IconChroma, NULL}, {NULL, NULL} };
static struct MenuAssets gAssets = {&BgTile, &Footer, &LabelEmpty, {&Tip0, &Tip1, &Tip2, NULL}};

static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(BetterflowAssets);

static Metadata M = Metadata()
    .title("Menu SDK Demo")
    .cubeRange(NUM_CUBES);


static void begin() {
    // Blank screens, attach VideoBuffers
    for(CubeID cube = 0; cube != NUM_CUBES; ++cube) {
        auto &vid = gVideo[cube];
        vid.initMode(BG0);
        vid.bg0.erase(BgTile);
        vid.attach(cube);
    }
}

void main() {
    begin();

    Menu m(gVideo[0], &gAssets, gItems);

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
                    LOG("found cube %d on side %d of menu (neighbor's %d side)\n",
                         e.neighbor.neighbor, e.neighbor.masterSide, e.neighbor.neighborSide);
                    break;
                case MENU_NEIGHBOR_REMOVE:
                    LOG("lost cube %d on side %d of menu (neighbor's %d side)\n",
                         e.neighbor.neighbor, e.neighbor.masterSide, e.neighbor.neighborSide);
                    break;

                case MENU_ITEM_ARRIVE:
                    LOG("arriving at menu item %d\n", e.item);
                    break;
                case MENU_ITEM_DEPART:
                    LOG("departing from menu item %d\n", e.item);
                    break;

                case MENU_PREPAINT:
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
        LOG("Selected Game: %d\n", e.item);
    }
}
