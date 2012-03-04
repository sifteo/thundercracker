#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

#define NUM_CUBES 3
#define LOAD_ASSETS 1
static Cube gCubes[NUM_CUBES];

typedef VidMode_BG0 Canvas;

#define NUM_ICONS 6
static const AssetImage* gIcons[NUM_ICONS] = {
	&Icon, &IconGreen, &IconBlue, &Icon, &IconGreen, &IconBlue
};

static void DrawClipped(Cube *pCube, Vec2 point, const Sifteo::AssetImage& asset, unsigned frame=0) {
	ASSERT( frame < asset.frames );
	if (point.x < 0) {
		int gutter = -point.x;
        _SYS_vbuf_wrect(
            &pCube->vbuf.sys, 
            Canvas(pCube->vbuf).BG0_addr(Vec2(point.x, point.y)) + gutter,
            asset.tiles + asset.width * asset.height * frame + gutter, 
            0,
            asset.width - gutter, 
            asset.height, 
            asset.width, 
            Canvas::BG0_width
        );
	} else {
        int extrema = point.x + asset.width;
        int clip = extrema - Canvas::BG0_width;
        _SYS_vbuf_wrect(
            &pCube->vbuf.sys, 
            Canvas(pCube->vbuf).BG0_addr(point), 
            asset.tiles + asset.width * asset.height * frame, 
            0,
            asset.width-(clip>0?clip:0), 
            asset.height, 
            asset.width, Canvas::BG0_width
        );
	}

}


unsigned unsigned_mod(int x, unsigned y) {
    // Modulo operator that always returns an unsigned result

    int z = x % (int)y;
    if (z < 0) z += y;
    return z;
}

// positions are in pixel units
// columns are in tile-units
// each icon is 80px/10tl wide with a 16px/2tl spacing

static void DrawColumn(Cube* pCube, int x) {
	// is this a blank column or an icon column?

    uint16_t addr = unsigned_mod(x, 18);
    x -= 3;
	uint16_t local_x = unsigned_mod(x, 12);
	Canvas g(pCube->vbuf);
	int iconId = (x +120)/ 12 - 10;
	if (local_x < 10 && iconId >= 0 && iconId < NUM_ICONS) {
		
		const AssetImage* pImg = gIcons[x / 12];
		const uint16_t *src = pImg->tiles + unsigned_mod(local_x, pImg->width);
		// drawing an icon column
		for(int row=0; row<10; ++row) {
	        _SYS_vbuf_writei(&pCube->vbuf.sys, addr, src, 0, 1);
    	    addr += 18;
        	src += pImg->width;

		}

	} else {
		
		// drawing a blank column
		for(int row=0; row<10; ++row) {
			g.BG0_drawAsset(Vec2(addr, row), BgTile);
		}

	}


	for(int row=0; row<10; ++row) {

	}
}

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
	canvas.setWindow(16,80);

    for (int x = -1; x < 17; x++) {
    	DrawColumn(pCube, x);
    }
    float t = 0;
	float u = 0;
	int prev_ut = 0;
	for(;;) {

		// do telem
		t += 0.15f * pCube->virtualAccel().x;
		if (t < 0.f) { t = 0.f; }
		else if (t > 96 * (NUM_ICONS-1)) { t = 96 * (NUM_ICONS-1); }
		u = 0.75f * u + + 0.25f * t;

		// update view
		int ui = u + 0.5f;
		int ut = u / 8;
		while(prev_ut < ut) {
			DrawColumn(pCube, prev_ut + 17);
			prev_ut++;
		}
		while(prev_ut > ut) {
			DrawColumn(pCube, prev_ut - 2);
			prev_ut--;
		}
		canvas.BG0_setPanning(Vec2(ui, 0));
		System::paint();
	}
}
