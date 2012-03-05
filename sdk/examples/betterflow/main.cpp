#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

#define NUM_CUBES 		3
#define LOAD_ASSETS		1
#define SFX_ON 			0
#define MUSIC_ON 		0
#define NUM_ICONS 		6

typedef VidMode_BG0 Canvas;

static Cube gCubes[NUM_CUBES];
static const AssetImage* gIcons[NUM_ICONS] = {
	&Icon, &IconGreen, &IconBlue, &Icon, &IconGreen, &IconBlue
};

// Modulo operator that always returns an unsigned result
inline unsigned UnsignedMod(int x, unsigned y) {
    const int z = x % (int)y;
    return z < 0 ? z+y : z;
}

inline float Lerp(float min, float max, float u) {
	return min + u * (max - min);
}

inline int ComputeSelected(float u) {
	int s = (u + (48.f))/96.f;
	return clamp(s, 0, NUM_ICONS-1);
}

inline float StoppingPositionFor(int selected) {
	return 96.f * (selected);
}

// positions are in pixel units
// columns are in tile-units
// each icon is 80px/10tl wide with a 16px/2tl spacing
static void DrawColumn(Cube* pCube, int x) {
	// x is the column in "global" space
    uint16_t addr = UnsignedMod(x, 18);
    x -= 3; // because the first icon is 24px inset
	const uint16_t local_x = UnsignedMod(x, 12);
	const int iconId = (x +120)/ 12 - 10; // weird math to avoid negative-floor
	// icon or blank column?
	if (local_x < 10 && iconId >= 0 && iconId < NUM_ICONS) {
		// drawing an icon column
		const AssetImage* pImg = gIcons[x / 12];
		const uint16_t *src = pImg->tiles + UnsignedMod(local_x, pImg->width);
		for(int row=0; row<10; ++row) {
	        _SYS_vbuf_writei(&pCube->vbuf.sys, addr, src, 0, 1);
    	    addr += 18;
        	src += pImg->width;

		}
	} else {
		// drawing a blank column
		Canvas g(pCube->vbuf);
		for(int row=0; row<10; ++row) {
			g.BG0_drawAsset(Vec2(addr, row), BgTile);
		}

	}
}

void siftmain() {
	// enable cube slots
	for (Cube *p = gCubes; p!=gCubes+NUM_CUBES; ++p) { p->enable(p-gCubes); }
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
	// initialize view
	Cube *pCube = gCubes;
	Canvas canvas(pCube->vbuf);
	canvas.setWindow(16,80);
    for (int x = -1; x < 17; x++) { DrawColumn(pCube, x); }
    // initialize physics
	float velocity = 0;
    float position = 0;
	int prev_ut = 0;


	for(;;) {
		// update physics
		float accel = 0.25f * pCube->virtualAccel().x;
		#if SIFTEO_SIMULATOR
			accel = -accel;
		#endif
		const float accelThreshold = 1.25f;
		const float dt = 0.225f;
		const bool isTilting = fabs(accel) > accelThreshold;
		const bool isLefty = position < 0.f - 0.05f;
		const bool isRighty = position > 96.f*(NUM_ICONS-1) + 0.05f;
		if (isTilting && !isLefty && !isRighty) {
			velocity += accel * dt;
			velocity *= 0.95f;
		} else {
			//if (isLefty && velocity < 0.f) { velocity = 0.f; }
			//if (isRighty && velocity > 0.f) { velocity = 0.f; }
			const float stiffness = 0.2f;
			const int selected = ComputeSelected(position);
			const float stopping_position = StoppingPositionFor(selected);
			velocity += stiffness * (stopping_position - position) * dt;
			velocity *= 0.825f;
		}
		position += velocity * dt;
		const float pad = 24.f;
		//position = clamp(position, -pad, 96.f*(NUM_ICONS-1)+pad);
		// update view
		int ui = position + 0.5f;
		int ut = position / 8;
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
