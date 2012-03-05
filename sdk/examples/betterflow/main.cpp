#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

// Constants
#define NUM_CUBES 		3
#define LOAD_ASSETS		1
#define SFX_ON 			0
#define MUSIC_ON 		0
#define NUM_ICONS 		4
#define NUM_TIPS		3

//typedef VidMode_BG0 Canvas;
typedef VidMode_BG0 Canvas;

// Static Globals
static Cube gCubes[NUM_CUBES];
static const AssetImage* gIcons[NUM_ICONS] = { &IconChroma, &IconSandwich, &IconPeano, &IconBuddy };
static const AssetImage* gTips[NUM_TIPS] = { &Tip0, &Tip1, &Tip2 };
static const AssetImage* gLabels[NUM_ICONS] = { &LabelChroma, &LabelSandwich, &LabelPeano, &LabelBuddy };

// Modulo operator that always returns an unsigned result
inline unsigned UnsignedMod(int x, unsigned y) { const int z = x % (int)y; return z < 0 ? z+y : z; }
inline float Lerp(float min, float max, float u) { return min + u * (max - min); }
inline int ComputeSelected(float u) { int s = (u + (48.f))/96.f; return clamp(s, 0, NUM_ICONS-1); }
inline float StoppingPositionFor(int selected) { return 96.f * (selected); }

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
		addr += 2*18;
		for(int row=0; row<10; ++row) {
	        _SYS_vbuf_writei(&pCube->vbuf.sys, addr, src, 0, 1);
    	    addr += 18;
        	src += pImg->width;

		}
	} else {
		// drawing a blank column
		Canvas g(pCube->vbuf);
		for(int row=0; row<10; ++row) {
			g.BG0_drawAsset(Vec2(addr, row+2), BgTile);
		}

	}
}

// wrapper for paint() that updates the footer
static int gCurrentTip = 0;
static float gPrevTime;
static void Paint(Cube *pCube) {
	float time = System::clock();
	float dt = time - gPrevTime;
	if (dt > 4.f) {
		gPrevTime = time - fmodf(dt, 4.f);
		const AssetImage& tip = *gTips[gCurrentTip];
        _SYS_vbuf_writei(
        	&pCube->vbuf.sys, 
        	offsetof(_SYSVideoRAM, bg1_tiles) / 2 + LabelEmpty.width * LabelEmpty.height,
            tip.tiles, 
            0, 
            tip.width * tip.height
        );
		gCurrentTip = (gCurrentTip+1) % NUM_TIPS;
	}
	System::paint();
}

// retrieve the acceleration of the cube due to tilting
const float kAccelThreshold = 0.85f;
static float GetAccel(Cube *pCube) {
	float accel = 0.25f * pCube->virtualAccel().x;
	#if SIFTEO_SIMULATOR
		accel = -accel;
	#endif
	return accel;
}

// entry point
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
	_SYS_vbuf_pokeb(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_BG1);
	Canvas canvas(pCube->vbuf);
    // Allocate tiles for the static upper label, and draw it.
    {
    	const AssetImage& label = *gLabels[0];
    	_SYS_vbuf_fill(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2, ((1 << label.width) - 1), label.height);
    	_SYS_vbuf_writei(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2, label.tiles, 0, label.width * label.height);
	}
	// Allocate tiles for the footer, and draw it.
    _SYS_vbuf_fill(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2 + 12, ((1 << Tip0.width) - 1), Tip0.height);
    _SYS_vbuf_writei(
    	&pCube->vbuf.sys, 
    	offsetof(_SYSVideoRAM, bg1_tiles) / 2 + LabelEmpty.width * LabelEmpty.height,
        Tip0.tiles, 
        0, 
        Tip0.width * Tip0.height
    );
    for (int x = -1; x < 17; x++) { DrawColumn(pCube, x); }
    Paint(pCube);
    // initialize physics
    float position = 0;
	int prev_ut = 0;
	gPrevTime = System::clock();
	for(;;) {
		// wait for a tilt or touch
		bool prevTouch = pCube->touching();
		while(fabs(GetAccel(pCube)) < kAccelThreshold) {
			Paint(pCube);
			bool touch = pCube->touching();
			// when touching any icon but the last one
			if (ComputeSelected(position) != NUM_ICONS-1 && touch && !prevTouch) {
				goto Selected;
			} else {
				prevTouch = touch;
			}

		}
		// hide label
	    _SYS_vbuf_writei(
	    	&pCube->vbuf.sys, 
	    	offsetof(_SYSVideoRAM, bg1_tiles) / 2, 
	    	LabelEmpty.tiles, 0, 
	    	LabelEmpty.width * 
	    	LabelEmpty.height
	    );
		bool doneTilting = false;
		float velocity = 0;
		position = StoppingPositionFor(ComputeSelected(position));
		while(!doneTilting) {
			// update physics
			const float accel = GetAccel(pCube);
			const float dt = 0.225f;
			const bool isTilting = fabs(accel) > kAccelThreshold;
			const bool isLefty = position < 0.f - 0.05f;
			const bool isRighty = position > 96.f*(NUM_ICONS-1) + 0.05f;
			if (isTilting && !isLefty && !isRighty) {
				velocity += accel * dt;
				velocity *= 0.99f;
			} else {
				const float stiffness = 0.333f;
				const int selected = ComputeSelected(position);
				const float stopping_position = StoppingPositionFor(selected);
				velocity += stiffness * (stopping_position - position) * dt;
				velocity *= 0.875f;
				doneTilting = fabs(velocity) < 1.0f && fabs(stopping_position - position) < 0.5f;
			}
			position += velocity * dt;
			const float pad = 24.f;
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
			Paint(pCube);
		}
		{
			// show the title of the game
			const AssetImage& label = *gLabels[ComputeSelected(position)];
		    _SYS_vbuf_writei(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2, label.tiles, 0, label.width * label.height);
		}
	}
	Selected:
	// hide bg1
	//_SYS_vbuf_fill(&pCube->vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2 + 12, 0, Tip0.height); // just footer
	canvas.set();
	
	// isolate the selected icon
	canvas.BG0_setPanning(Vec2(0,0));
	for(int row=0; row<18; ++row)
	for(int col=0; col<18; ++col) {
		canvas.BG0_drawAsset(Vec2(col, row), BgTile);
	}
	canvas.BG0_drawAsset(Vec2(3,2), *gIcons[ComputeSelected(position)]);
	System::paintSync();
	System::paintSync();

	// scroll-out icon with a parabolic arc
	const float k = 5.f;
	int pany = 0;
	int i=0;
	int prevBottom = 12;
	while(pany>-128) {
		i++;
		float u = i/30.f;
		u = (1.-k*u);
		pany = int(8*(1.f-u*u));
		if (pany < -32) {
			// TODO: make this math a little less hacky.  I think sometimes
			// I cut off the bottom row before it's offscreen :(
			int newBottom = 15 + (pany/8);
			while(newBottom < prevBottom && prevBottom>=0) {
				for(int col=0; col<18; ++col) {
					canvas.BG0_drawAsset(Vec2(col, prevBottom), BgTile);
				}
				prevBottom--;
			}
		}
		canvas.BG0_setPanning(Vec2(0, pany));
		System::paint();
	}
	// TODO: actually choose game
	LOG(("Selected Game: %d\n", ComputeSelected(position)));
	for(;;) { System::paint(); }
}
