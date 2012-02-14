#include "Game.h"
#include "Dialog.h"
#include "DrawingHelpers.h"

Cube gCubes[NUM_CUBES];
AudioChannel gChannelSfx;
AudioChannel gChannelMusic;

//static Game sGame;
Game* pGame = 0;

Cube* IntroCutscene();
void WinScreen(Cube* primaryCube);

static void ScrollMap(const MapData* pMap) {
	int x = 0;
	int y = 0;
	ViewMode gfx(gCubes->vbuf);
	gfx.init();
	while(1) {
		Vec2 accel = gCubes->virtualAccel();
		x += accel.x / 10;
		y += accel.y / 10;
		DrawOffsetMap(&gfx, pMap, Vec2(x,y));
		System::paintSync();
	}
}

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
	{ // initialize assets
	  for (Cube::ID i = 0; i < NUM_CUBES; i++) {
      gCubes[i].loadAssets(GameAssets);
	    VidMode_BG0_ROM rom(gCubes[i].vbuf);
	    rom.init();
	    rom.BG0_text(Vec2(1,1), "Loading...");
	  }
	  bool done = false;
	  while(!done) {
	  	done = true;
	    for (Cube::ID i = 0; i < NUM_CUBES; i++) {
	      VidMode_BG0_ROM rom(gCubes[i].vbuf);
	      rom.BG0_progressBar(Vec2(0,7), gCubes[i].assetProgress(GameAssets, VidMode_BG0::LCD_width), 2);
	      done &= gCubes[i].assetDone(GameAssets);
	    }
	    System::paint();
	  }
	}
	#endif
	#if SFX_ON
	gChannelSfx.init();
	#endif
	#if MUSIC_ON
	gChannelMusic.init();
	#endif

	//ScrollMap(gMapData+1);

	for(;;) {
		//#ifndef SIFTEO_SIMULATOR
		PlayMusic(music_sting, false);
		Cube* pPrimary = IntroCutscene();
		//#else 
		//Cube* pPrimary = gCubes;
		//#endif
		{
			Game game;
			pGame = &game;
			game.MainLoop(pPrimary);
			pGame = 0;
		}
		for(unsigned i=0; i<100; ++i) { System::paint(); }
		//PlayMusic(music_winscreen, false);
		//WinScreen(gCubes);
	}
}

//-----------------------------------------------------------------------------
// SCROLL FX
//-----------------------------------------------------------------------------

static void WaitForSeconds(float dt) {
	float t = System::clock();
	do { System::paint(); } while(System::clock() - t < dt);
}

Cube* IntroCutscene() {
	for(unsigned i=0; i<NUM_CUBES; ++i) {
		ViewMode gfx(gCubes[i].vbuf);
		gfx.set();
		gfx.BG0_drawAsset(Vec2(0,0), Sting);
		for(unsigned s=0; s<8; ++s) {
			gfx.hideSprite(s);
		}
		BG1Helper overlay(gCubes[i]);
		overlay.DrawAsset(Vec2(0,0), Title);
		overlay.Flush();
		gfx.BG1_setPanning(Vec2(0,64));
		gCubes[i].vbuf.touch();
	}
	float dt;
	for(float t=System::clock(); (dt=(System::clock()-t))<0.9f;) {
		float u = 1.f - (dt / 0.9f);
		u = 1.f - (u*u*u*u);
		for(unsigned i=0; i<NUM_CUBES; ++i) {
			ViewMode(gCubes[i].vbuf).BG1_setPanning(Vec2(0, 72 - 128*u));
		}
		System::paintSync();
	}
	
	// wait for a touch
	Cube* pCube = 0;
	while(!pCube) {
		for(unsigned i=0; i<NUM_CUBES; ++i) {
			if (gCubes[i].touching()) {
				pCube = gCubes + i;
				PlaySfx(sfx_neighbor);
				break;
			}
		}
		System::yield();
	}
	// blank other cubes
	for(unsigned i=0; i<NUM_CUBES; ++i) {
		if (gCubes+i != pCube) {
			BG1Helper(gCubes[i]).Flush();
			ViewMode gfx(gCubes[i].vbuf);
			gfx.BG1_setPanning(Vec2(0,0));
			gfx.BG0_drawAsset(Vec2(0,0), Blank);
		}
	}
	ViewMode mode(pCube->vbuf);
	// hide banner
	for(float t=System::clock(); (dt=(System::clock()-t))<0.9f;) {
		float u = 1.f - (dt / 0.9f);
		u = 1.f - (u*u*u*u);
		mode.BG1_setPanning(Vec2(0, -56 + 128*u));
		System::paintSync();
	}
	WaitForSeconds(0.1f);
	// pearl walks up from bottom
	PlaySfx(sfx_running);
	int framesPerCycle = PlayerWalk.frames >> 2;
	int tilesPerFrame = PlayerWalk.width * PlayerWalk.height;
	mode.resizeSprite(0, 32, 32);
	for(unsigned i=0; i<48/2; ++i) {
		mode.setSpriteImage(0, PlayerWalk.index + tilesPerFrame * (i%framesPerCycle));
		mode.moveSprite(0, 64-16, 128-i-i);
		System::paintSync();
	}
	// face front
	mode.setSpriteImage(0, PlayerStand.index + SIDE_BOTTOM * (PlayerStand.width * PlayerStand.height));
	WaitForSeconds(0.5f);

	// look left
	mode.setSpriteImage(0, PlayerIdle.index);
	WaitForSeconds(0.5f);
	mode.setSpriteImage(0, PlayerStand.index + SIDE_BOTTOM * (PlayerStand.width * PlayerStand.height));
	WaitForSeconds(0.5f);

	// look right
	mode.setSpriteImage(0, PlayerIdle.index + (PlayerIdle.width * PlayerIdle.height));
	WaitForSeconds(0.5f);
	mode.setSpriteImage(0, PlayerStand.index + SIDE_BOTTOM * (PlayerStand.width * PlayerStand.height));
	WaitForSeconds(0.5f);

	// thought bubble appears
	mode.BG0_drawAsset(Vec2(10,8), TitleThoughts);
	WaitForSeconds(0.5f);
	mode.BG0_drawAsset(Vec2(3,4), TitleBalloon);
	WaitForSeconds(0.5f);

	// items appear
	const int pad = 36;
	const int innerPad = (128 - pad - pad) / 3;
	for(unsigned i = 0; i < 4; ++i) {
		int x = pad + innerPad * i - 8;
		mode.setSpriteImage(i+1, Items.index + Items.width * Items.height * (i+1));
		mode.resizeSprite(i+1, 16, 16);
		// jump
		//PlaySfx(sfx_pickup);
		for(int j=0; j<6; j++) {
			mode.moveSprite(i+1, x, 42 - j);
			System::paint();
		}
		for(int j=6; j>0; --j) {
			mode.moveSprite(i+1, x, 42 - j);
			System::paint();
		}
		mode.moveSprite(i+1, x, 42);
		System::paintSync();
	}
	WaitForSeconds(1.f);

	// do the pickup animation
	for(unsigned i=0; i<PlayerPickup.frames; ++i) {
		mode.setSpriteImage(0, PlayerPickup.index + i * PlayerPickup.width * PlayerPickup.height);
		System::paintSync();
		WaitForSeconds(0.05f);
	}
	mode.setSpriteImage(0, PlayerStand.index + SIDE_BOTTOM * (PlayerStand.width * PlayerStand.height));
	WaitForSeconds(2.f);

	// hide items and bubble
	mode.hideSprite(1);
	mode.hideSprite(2);
	mode.hideSprite(3);
	mode.hideSprite(4);
	mode.BG0_drawAsset(Vec2(0,0), Sting);
	System::paintSync();

	// walk off
	PlaySfx(sfx_running);
	unsigned downIndex = PlayerWalk.index + SIDE_BOTTOM * tilesPerFrame * framesPerCycle;
	for(unsigned i=0; i<76/2; ++i) {
		mode.setSpriteImage(0, PlayerWalk.index + tilesPerFrame * (i%framesPerCycle));
		mode.moveSprite(0, 64-16, 80-i-i);
		System::paintSync();
	}
	mode.hideSprite(0);

	// iris out
	for(unsigned i=0; i<8; ++i) {
		for(unsigned x=i; x<16-i; ++x) {
			mode.BG0_putTile(Vec2(x, i), *Black.tiles);
			mode.BG0_putTile(Vec2(x, 16-i-1), *Black.tiles);
		}
		for(unsigned y=i+1; y<16-i-1; ++y) {
			mode.BG0_putTile(Vec2(i, y), *Black.tiles);
			mode.BG0_putTile(Vec2(16-i-1, y), *Black.tiles);
		}
		System::paintSync();
	}
	BG1Helper(*pCube).Flush();
	ViewMode(pCube->vbuf).BG1_setPanning(Vec2(0,0));
	WaitForSeconds(0.5f);
	return pCube;
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