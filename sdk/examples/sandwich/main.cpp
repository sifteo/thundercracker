#include "Game.h"
#include "Dialog.h"

Cube gCubes[NUM_CUBES];
AudioChannel gChannelSfx;
AudioChannel gChannelMusic;

static Game sGame;
Game* pGame = 0;


static bool AnyNeighbors(const Cube& c) {
	return c.hasPhysicalNeighborAt(0) || 
		c.hasPhysicalNeighborAt(1) || 
		c.hasPhysicalNeighborAt(2) || 
		c.hasPhysicalNeighborAt(3);
}

void siftmain() {
	for (Cube::ID i = 0; i < NUM_CUBES; i++) {
    	gCubes[i].enable(i + CUBE_ID_BASE);
  	}
	#if LOAD_ASSETS
	  for (Cube::ID i = 0; i < NUM_CUBES; i++) {
      	gCubes[i].loadAssets(SandwichAssets);
	    VidMode_BG0_ROM rom(gCubes[i].vbuf);
	    rom.init();
	    rom.BG0_text(Vec2(1,1), "Loading...");
	  }
	  bool done = false;
	  while(!done) {
	  	done = true;
	    for (Cube::ID i = 0; i < NUM_CUBES; i++) {
	      VidMode_BG0_ROM rom(gCubes[i].vbuf);
	      rom.BG0_progressBar(Vec2(0,7), gCubes[i].assetProgress(SandwichAssets, VidMode_BG0::LCD_width), 2);
	      done &= gCubes[i].assetDone(SandwichAssets);
	    }
	    System::paint();
	  }
	#endif
	#if SFX_ON
		gChannelSfx.init();
	#endif
	#if MUSIC_ON
		gChannelMusic.init();
	#endif
	while(1) {
		pGame = &sGame;
		pGame->MainLoop();
		pGame = 0;
	}
}
/*
//-----------------------------------------------------------------------------
// WIN SCREEN FX
//-----------------------------------------------------------------------------

struct Spartikle {
	unsigned frame;
	float px;
	float py;
	float vx;
	float vy;

	inline void Update(float dt, Cube* c, int id) {
		vy += dt * 9.8f;
		px += dt * vx;
		py += dt * vy;
		if (py > 134.0f) {
			py = 268.0f - py;
			vy *= -0.5f;
		}
		frame = (frame+1)%4;
		VidMode_BG0_SPR_BG1 mode(c->vbuf);
		mode.setSpriteImage(id, Sparkle.index + frame);
		mode.moveSprite(id, (int)(px+0.5f)-4, (int)(py+0.5f)-4);
	}
};

void WinScreen(Cube* primaryCube) {
	VidMode_BG0_SPR_BG1 mode(primaryCube->vbuf);
	mode.set();
	mode.clear();

	for(unsigned i=0; i<8; ++i) { mode.hideSprite(i); }
	mode.BG0_drawAsset(Vec2(0,0), WinscreenBackground);
	for(unsigned i=0; i<NUM_CUBES; ++i) {
		if (gCubes+i != primaryCube) {
			VidMode_BG0 idleMode(gCubes[i].vbuf);
			idleMode.init();
			idleMode.BG0_drawAsset(Vec2(0,0), Sting);
		}
	}
    for(unsigned frame=0; frame<WinscreenAnim.frames; ++frame) {
  	    BG1Helper mode(*primaryCube);
  	    mode.DrawAsset(Vec2(9, 0), WinscreenAnim, frame);
  	    mode.Flush();
    	System::paintSync();
    }
	{
		VidMode_BG0 mode(primaryCube->vbuf);
		for(unsigned row=0; row<8; ++row) {
			for(unsigned col=0; col<16; ++col) {
				mode.BG0_drawAsset(Vec2(col,8+row), Flash);
				mode.BG0_drawAsset(Vec2(col,8-row-1), Flash);
			}
			if (row%2 == 1) {
				System::paintSync();
			}
		}
		for(unsigned row=0; row<8; ++row) {
			unsigned top = 7-row;
			unsigned height = 2 * (row+1);
			mode.BG0_drawPartialAsset(Vec2(0, top), Vec2(0, top), Vec2(16, height), Winscreen);
			if (row%2 == 1) {
				System::paintSync();
			}
		}
    }
	BG1Helper(*primaryCube).Flush();
	VidMode_BG0(primaryCube->vbuf).BG0_drawAsset(Vec2(0,0), Winscreen);

    // sparkles
    Spartikle sp[4] = {
    	{ 0, 91, 39, -10, -8 },
    	{ 2, 111, 63, -5, -12 },
    	{ 1, 80, 66, -2, -10 },
    	{ 3, 89, 50, 3, -8 },
    };
    for(unsigned id=0; id<4; ++id) {
    	sp[id].vx *= 1.5f;
    	sp[id].vy *= 2.f;
    	mode.resizeSprite(id, 8, 8);
    }
    float t = System::clock();
    do {
	    for(unsigned id=0; id<4; ++id) { sp[id].Update(0.2f, primaryCube, id); }
    	System::paint();
    } while(System::clock() - t < 2.5f);
    for(unsigned id=0; id<8; ++id) {
    	mode.hideSprite(id);
    }
    System::paintSync();
    WaitForSeconds(2.f);
}
*/