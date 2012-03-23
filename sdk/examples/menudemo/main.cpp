#include <sifteo.h>
#include "assets.gen.h"
#include <sifteo/menu.h>
using namespace Sifteo;

// Constants
#define NUM_CUBES 			3
#define LOAD_ASSETS			1

#define NUM_ITEMS 			4
#define NUM_TIPS			3

// Static Globals
static Cube gCubes[NUM_CUBES];
static struct MenuItem gItems[NUM_ITEMS + 1] = { {&IconChroma, &LabelChroma}, {&IconSandwich, &LabelSandwich}, {&IconPeano, &LabelPeano}, {&IconBuddy, &LabelBuddy} };
static struct MenuAssets gAssets = {&BgTile, &Footer, &LabelEmpty, {&Tip0, &Tip1, &Tip2, NULL}};

static void begin() {
	// enable cube slots
	for (Cube *p = gCubes; p!=gCubes+NUM_CUBES; ++p) { p->enable(p-gCubes); }
  	// load assets
	#if LOAD_ASSETS
		for(Cube *p = gCubes; p!=gCubes+NUM_CUBES; ++p) {
			p->loadAssets(BetterflowAssets);
			VidMode_BG0_ROM rom(p->vbuf);
			rom.init();
			rom.BG0_text(Vec2(1,1), "Loading...");
		}
		bool done = false;
		while(!done) {
			done = true;
			for(Cube *p = gCubes; p!=gCubes+NUM_CUBES; ++p) {
				VidMode_BG0_ROM rom(p->vbuf);
				rom.BG0_progressBar(Vec2(0,7), p->assetProgress(BetterflowAssets, VidMode_BG0::LCD_width), 2);
				done &= p->assetDone(BetterflowAssets);
			}
			System::paint();
		}
	#endif
	// blank screens
	for(Cube* p=gCubes; p!=gCubes+NUM_CUBES; ++p) {
		VidMode_BG0 g(p->vbuf);
		g.set();
		g.clear();
		for(unsigned r=0; r<18; ++r)
		for(unsigned c=0; c<18; ++c) {
			g.BG0_drawAsset(Vec2(c,r), BgTile);
		}

	}
	// sync up
	System::paintSync();
	for(Cube* p=gCubes; p!=gCubes+NUM_CUBES; ++p) { p->vbuf.touch(); }
	System::paintSync();
	for(Cube* p=gCubes; p!=gCubes+NUM_CUBES; ++p) { p->vbuf.touch(); }
	System::paintSync();
}

// entry point
void main() {

	begin();
	

	Menu m(&gCubes[0], &gAssets, gItems);
	
	struct MenuEvent e;
	while(1) {
		while(m.pollEvent(&e)) {
			switch(e.type) {
				case MENU_ITEM_PRESS:
					// Game Buddy is not clickable, so don't do anything on press
					if(e.item == 3) {
						m.preventDefault();
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

				case MENU_PREPAINT: {
					// do your implementation-specific drawing here
					// NOTE: this event should never have its default handler skipped.
					break;
				}
				case MENU_UNEVENTFUL:
					// this should never happen. if it does, it can/should be ignored.
					ASSERT(false);
					break;
			}
		}
		ASSERT(e.type == MENU_EXIT);
		LOG(("Selected Game: %d\n", e.item));
	}
}
