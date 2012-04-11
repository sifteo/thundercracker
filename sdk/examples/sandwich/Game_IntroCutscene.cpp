#include "Game.h"

VideoBuffer* Game::IntroCutscene() {
	for(unsigned i=0; i<NUM_CUBES; ++i) {
		VideoBuffer& gfx = ViewAt(i).Canvas();
		gfx.initMode(BG0_SPR_BG1);
		gfx.bg0.image(vec(0,0), Sting);
		for(unsigned s=0; s<8; ++s) {
			gfx.sprites[s].hide();
		}
		// TODO
		//BG1Helper overlay(gCubes[i]);
		//overlay.DrawAsset(vec(0,0), SandwichTitle);
		//overlay.Flush();
		gfx.bg1.setPanning(vec(0,64));
	}
	float dt;
	for(SystemTime t=SystemTime::now(); (dt=(SystemTime::now()-t))<0.9f;) {
		float u = 1.f - (dt / 0.9f);
		u = 1.f - (u*u*u*u);
		for(unsigned i=0; i<NUM_CUBES; ++i) {
			ViewAt(i).Canvas().bg1.setPanning(vec(0.f, 72 - 128*u));
		}
		DoPaint();
	}
	
	// wait for a touch
	CubeID cube = 0;
	bool hasTouch = false;
	while(!hasTouch) {
		for(unsigned i=0; i<NUM_CUBES; ++i) {
			cube = i;
			if (cube.isTouching()) {
				PlaySfx(sfx_neighbor);
				hasTouch = true;
				break;
			}
		}
		System::yield();
	}
	// blank other cubes
	for(unsigned i=0; i<NUM_CUBES; ++i) {
		CubeID c = i;
		if (c != cube) {
			VideoBuffer& gfx = ViewAt(i).Canvas();
			gfx.bg1.eraseMask(false);
			gfx.bg1.setPanning(vec(0,0));
			gfx.bg0.image(vec(0,0), Blank);
		}
	}
	VideoBuffer& mode = ViewAt(cube).Canvas();
	// hide banner
	for(SystemTime t=SystemTime::now(); (dt=(SystemTime::now()-t))<0.9f;) {
		float u = 1.f - (dt / 0.9f);
		u = 1.f - (u*u*u*u);
		mode.bg1.setPanning(vec(0.f, -56 + 128*u));
		DoPaint();
	}
	WaitForSeconds(0.1f);
	// pearl walks up from bottom
	PlaySfx(sfx_running);
	int framesPerCycle = PlayerWalk.numFrames() >> 2;
	int tilesPerFrame = PlayerWalk.numTilesPerFrame();
	mode.sprites[0].resize(32, 32);
	for(unsigned i=0; i<48/2; ++i) {
		mode.sprites[0].setImage(PlayerWalk.tile(0) + tilesPerFrame * (i%framesPerCycle));
		mode.sprites[0].move(64-16, 128-i-i);
		DoPaint();
	}
	// face front
	mode.sprites[0].setImage(PlayerStand.tile(0) + BOTTOM * (PlayerStand.numTilesPerFrame()));
	WaitForSeconds(0.5f);

	// look left
	mode.sprites[0].setImage(PlayerIdle.tile(0));
	WaitForSeconds(0.5f);
	mode.sprites[0].setImage(PlayerStand.tile(0) + BOTTOM * (PlayerStand.numTilesPerFrame()));
	WaitForSeconds(0.5f);

	// look right
	mode.sprites[0].setImage(PlayerIdle.tile(0) + (PlayerIdle.numTilesPerFrame()));
	WaitForSeconds(0.5f);
	mode.sprites[0].setImage(PlayerStand.tile(0) + BOTTOM * (PlayerStand.numTilesPerFrame()));
	WaitForSeconds(0.5f);

	// thought bubble appears
	mode.bg0.image(vec(10,8), TitleThoughts);
	WaitForSeconds(0.5f);
	mode.bg0.image(vec(3,4), TitleBalloon);
	WaitForSeconds(0.5f);

	// items appear
	const int pad = 36;
	const int innerPad = (128 - pad - pad) / 3;
	for(unsigned i = 0; i < 4; ++i) {
		int x = pad + innerPad * i - 8;
		mode.sprites[i+1].setImage(Items, i);
		mode.sprites[i+1].resize(16, 16);
		// jump
		//PlaySfx(sfx_pickup);
		for(int j=0; j<6; j++) {
			mode.sprites[i+1].move(x, 42 - j);
			DoPaint();
		}
		for(int j=6; j>0; --j) {
			mode.sprites[i+1].move(x, 42 - j);
			DoPaint();
		}
		mode.sprites[i+1].move(x, 42);
		DoPaint();
	}
	WaitForSeconds(1.f);

	// do the pickup animation
	for(unsigned i=0; i<PlayerPickup.numFrames(); ++i) {
		mode.sprites[0].setImage(PlayerPickup.tile(0) + i * PlayerPickup.numTilesPerFrame());
		DoPaint();
		WaitForSeconds(0.05f);
	}
	mode.sprites[0].setImage(PlayerStand.tile(0) + BOTTOM * (PlayerStand.numTilesPerFrame()));
	WaitForSeconds(2.f);

	// hide items and bubble
	mode.sprites[1].hide();
	mode.sprites[2].hide();
	mode.sprites[3].hide();
	mode.sprites[4].hide();
	mode.bg0.image(vec(0,0), Sting);
	DoPaint();

	// walk off
	PlaySfx(sfx_running);
	unsigned downIndex = PlayerWalk.tile(0) + BOTTOM * tilesPerFrame * framesPerCycle;
	for(unsigned i=0; i<76/2; ++i) {
		mode.sprites[0].setImage(PlayerWalk.tile(0) + tilesPerFrame * (i%framesPerCycle));
		mode.sprites[0].move(64-16, 80-i-i);
		DoPaint();
	}
	mode.sprites[0].hide();

	// iris out
	for(unsigned i=0; i<8; ++i) {
		for(unsigned x=i; x<16-i; ++x) {
			mode.bg0.image(vec(x, i), BlackTile);
			mode.bg0.image(vec(x, 16-i-1), BlackTile);
		}
		for(unsigned y=i+1; y<16-i-1; ++y) {
			mode.bg0.image(vec(i, y), BlackTile);
			mode.bg0.image(vec(16-i-1, y), BlackTile);
		}
		DoPaint();
	}
	WaitForSeconds(0.5f);
	return &ViewAt(cube).Canvas();
}
