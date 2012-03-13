#include <sifteo.h>
#include <sifteo/menu.h>
#include "assets.gen.h"
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
void siftmain() {
	begin();
	
	Menu m(&gCubes[0], &gAssets, gItems);
	
	struct MenuEvent e;
	while(1) {
		m.pollEvent(&e);
		// if(e.type == MENU_PREPAINT) {
		// 	m.paintSync();
		// }
		switch(e.type) {
			case MENU_EXIT:
				LOG(("Selected Game: %d\n", e.item));
				break;
			case MENU_ITEM_PRESS:
				// Buddy is not here yet, so don't do anything on press
				if(e.item == 3) {
					m.preventDefault();
				}
				break;
			case MENU_NEIGHBOR_ADD:
			case MENU_NEIGHBOR_REMOVE:
			case MENU_ITEM_ARRIVE:
			case MENU_ITEM_DEPART:
			case MENU_PREPAINT:
			default:
				break;
		}
	}
}
