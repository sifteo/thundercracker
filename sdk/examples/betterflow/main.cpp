#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

#define NUM_CUBES 3
#define LOAD_ASSETS 1
static Cube gCubes[NUM_CUBES];

typedef VidMode_BG0_SPR_BG1 Canvas;

void siftmain() {

	// enable cube slots
	for (Cube *p = gCubes; p!=gCubes+NUM_CUBES; ++p) {
    	p->enable(p-gCubes);
  	}

  	// load assets
	#if LOAD_ASSETS
	for(Cube *p = gCubes; p!=gCubes+NUM_CUBES; ++p) {
		p->loadAssets(BetterflowAssets);
		VidMode_BG0_ROM rom(p->vbuf);
		rom.init();
		rom.BG0_text(Vec2(1,1), "Loading!?!");
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
		Canvas g(p->vbuf);
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
	
	Cube *pCube = gCubes;
	Canvas canvas(pCube->vbuf);
	canvas.setWindow(24,80);
	canvas.BG0_drawAsset(Vec2(3,0), Icon);

	while(1) {

		System::paint();
	}
}
