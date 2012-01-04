#include "Game.h"

Cube gCubes[NUM_CUBES];
AudioChannel gChannelSfx;

static Game sGame;
Game* pGame = &sGame;

void IntroCutscene();
void WinScreen(Cube* primaryCube);

void siftmain() {
	{ // initialize assets
	  for (Cube::ID i = 0; i < NUM_CUBES; i++) {
	    gCubes[i].enable(i);
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
	gChannelSfx.init();
	for(;;) {
		//IntroCutscene();
		*pGame = Game(); // re-initialize memory
		pGame->MainLoop();
		WinScreen(pGame->player.CurrentView()->GetCube());
	}
}

//-----------------------------------------------------------------------------
// SCROLL FX
//-----------------------------------------------------------------------------

static void WaitForSeconds(float dt) {
	float t = System::clock();
	do { System::paint(); } while(System::clock() - t < dt);
}

void IntroCutscene() {
	for(int i=0; i<NUM_CUBES; ++i) {
		VidMode_BG0 stingMode(gCubes[i].vbuf);
		stingMode.init();
		stingMode.BG0_drawAsset(Vec2(0,0), Sting);
		gCubes[i].vbuf.touch();
	}
	//System::paintSync();
	WaitForSeconds(2.f);
	EnterSpriteMode(&gCubes[0]);
	VidMode_BG0 mode(gCubes[0].vbuf);

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
	// scroll transitions in
	for(unsigned i=1; i<=16; ++i) {
		mode.BG0_drawPartialAsset(Vec2(6,16-i), Vec2(0,0), Vec2(2,i), ScrollLeft);
		mode.BG0_drawPartialAsset(Vec2(8,16-i), Vec2(0,0), Vec2(2,i), ScrollRight);
		System::paintSync();
	}
	// scroll opens
	mode.BG0_drawAsset(Vec2(5,0), ScrollLeft);
	mode.BG0_drawAsset(Vec2(7,0), ScrollLeftAccent);
	mode.BG0_drawAsset(Vec2(8,0), ScrollRightAccent);
	mode.BG0_drawAsset(Vec2(9,0), ScrollRight);
	System::paintSync();
	for(unsigned i=2; i<7; ++i) {
		mode.BG0_drawAsset(Vec2(6-i,0), ScrollLeft);
		mode.BG0_drawAsset(Vec2(6-i+2,0), ScrollLeftAccent);
		mode.BG0_drawAsset(Vec2(6-i+3,0), ScrollMiddle);
		mode.BG0_drawAsset(Vec2(8+i-2,0), ScrollMiddle);
		mode.BG0_drawAsset(Vec2(8+i-1,0), ScrollRightAccent);
		mode.BG0_drawAsset(Vec2(8+i,0), ScrollRight);
		System::paintSync();
	}

	// pearl walks up from bottom
	int framesPerCycle = PlayerWalk.frames >> 2;
	int tilesPerFrame = PlayerWalk.width * PlayerWalk.height;
	ResizeSprite(&gCubes[0], 0, 32, 32);
	for(unsigned i=0; i<48; ++i) {
		SetSpriteImage(&gCubes[0], 0, PlayerWalk.index + tilesPerFrame * (i%framesPerCycle));
		MoveSprite(&gCubes[0], 0, 64-16, 128-i);
		System::paintSync();
	}
	// face front
	SetSpriteImage(&gCubes[0], 0, PlayerStand.index + SIDE_BOTTOM * (PlayerStand.width * PlayerStand.height));
	WaitForSeconds(0.5f);

	// look left
	SetSpriteImage(&gCubes[0], 0, PlayerIdle.index);
	WaitForSeconds(0.5f);
	SetSpriteImage(&gCubes[0], 0, PlayerStand.index + SIDE_BOTTOM * (PlayerStand.width * PlayerStand.height));
	WaitForSeconds(0.5f);

	// look right
	SetSpriteImage(&gCubes[0], 0, PlayerIdle.index + (PlayerIdle.width * PlayerIdle.height));
	WaitForSeconds(0.5f);
	SetSpriteImage(&gCubes[0], 0, PlayerStand.index + SIDE_BOTTOM * (PlayerStand.width * PlayerStand.height));
	WaitForSeconds(0.5f);

	// thought bubble appears
	mode.BG0_drawAsset(Vec2(8,8), ScrollThoughts);
	WaitForSeconds(0.5f);
	mode.BG0_drawAsset(Vec2(3,4), ScrollBubble);
	WaitForSeconds(0.5f);

	// items appear
	const int pad = 36;
	const int innerPad = (128 - pad - pad) / 3;
	for(unsigned i = 0; i < 4; ++i) {
		int x = pad + innerPad * i - 8;
		SetSpriteImage(&gCubes[0], i+1, Items.index + Items.width * Items.height * (i+1));
		ResizeSprite(&gCubes[0], i+1, 16, 16);
		// jump
		for(int j=0; j<6; j++) {
			MoveSprite(&gCubes[0], i+1, x, 42 - j);
			System::paint();
		}
		for(int j=6; j>0; --j) {
			MoveSprite(&gCubes[0], i+1, x, 42 - j);
			System::paint();
		}
		MoveSprite(&gCubes[0], i+1, x, 42);
		System::paintSync();
	}
	WaitForSeconds(1.f);

	// do the pickup animation
	for(unsigned i=0; i<PlayerPickup.frames; ++i) {
		SetSpriteImage(&gCubes[0], 0, PlayerPickup.index + i * PlayerPickup.width * PlayerPickup.height);
		System::paintSync();
		WaitForSeconds(0.05f);
	}
	SetSpriteImage(&gCubes[0], 0, PlayerStand.index + SIDE_BOTTOM * (PlayerStand.width * PlayerStand.height));
	WaitForSeconds(2.f);

	// hide items and bubble
	HideSprite(&gCubes[0], 1);
	HideSprite(&gCubes[0], 2);
	HideSprite(&gCubes[0], 3);
	HideSprite(&gCubes[0], 4);
	for(int x=3; x<13; ++x) { mode.BG0_drawAsset(Vec2(x, 0), ScrollMiddle); }
	System::paintSync();

	// walk off
	unsigned downIndex = PlayerWalk.index + SIDE_BOTTOM * tilesPerFrame * framesPerCycle;
	for(unsigned i=48; i>0; --i) {
		SetSpriteImage(&gCubes[0], 0, downIndex + tilesPerFrame * (i%framesPerCycle));
		MoveSprite(&gCubes[0], 0, 64-16, 128-i);
		System::paintSync();
	}
	HideSprite(&gCubes[0], 0);

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

	WaitForSeconds(0.5f);
}

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
		SetSpriteImage(c, id, Sparkle.index + frame);
		MoveSprite(c, id, (int)(px+0.5f)-4, (int)(py+0.5f)-4);
	}
};

void WinScreen(Cube* primaryCube) {
	EnterSpriteMode(primaryCube);
	for(unsigned i=0; i<8; ++i) { HideSprite(primaryCube, i); }
	VidMode_BG0(primaryCube->vbuf).BG0_drawAsset(Vec2(0,0), WinscreenBackground);
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
    	ResizeSprite(primaryCube, id, 8, 8);
    }
    float t = System::clock();
    do {
	    for(unsigned id=0; id<4; ++id) { sp[id].Update(0.2f, primaryCube, id); }
    	System::paint();
    } while(System::clock() - t < 2.5f);
    for(unsigned id=0; id<8; ++id) {
    	HideSprite(primaryCube, id);
    }
    System::paintSync();
    WaitForSeconds(2.f);
}