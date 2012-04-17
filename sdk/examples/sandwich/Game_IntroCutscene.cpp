#include "Game.h"

Viewport* Game::IntroCutscene() {

	for(auto& view : views) {
		auto& gfx = view.Canvas();
		gfx.initMode(BG0_SPR_BG1);
		gfx.bg0.image(vec(0,0), Sting);
		for(unsigned s=0; s<8; ++s) {
			gfx.sprites[s].hide();
		}
		gfx.bg1.fillMask(vec(0,0), vec(128,64)/8);
		gfx.bg1.image(vec(0,0), SandwichTitle);
		gfx.bg1.setPanning(vec(0,64));
	}

	auto startTime = SystemTime::now();
	auto deadline = startTime + 0.9f;
	while(deadline.inFuture()) {
		float u = 1.f - float(SystemTime::now() - startTime) / 0.9f;
		u = 1.f - (u*u*u*u);
		for(auto& view : views) {
			view.Canvas().bg1.setPanning(vec(0.f, 72 - 128*u));
		}
		DoPaint();
	}

	// wait for a touch
	CubeID cube = 0;
	bool hasTouch = false;
	while(!hasTouch) {
		for(auto& view : views) {
			cube = view.GetID();
			if (cube.isTouching()) {
				PlaySfx(sfx_neighbor);
				hasTouch = true;
				break;
			}
		}
		System::paint();
	}
	// blank other cubes
	for(auto& view : views) {
		CubeID c = view.GetID();
		if (c != cube) {
			auto& gfx = view.Canvas();
			gfx.bg1.erase();
			gfx.bg1.setPanning(vec(0,0));
			gfx.bg0.image(vec(0,0), Blank);
		}
	}
	auto& g = ViewAt(cube).Canvas();
	// hide banner
	startTime = SystemTime::now();
	deadline = startTime + 0.9f;
	while(deadline.inFuture()) {
		float u = 1.f - (float(SystemTime::now() - startTime) / 0.9f);
		u = 1.f - (u*u*u*u);
		g.bg1.setPanning(vec(0.f, -56 + 128*u));
		DoPaint();
	}
	g.bg1.erase();
	DoWait(0.1f);

	//
	// pearl walks up from bottom
	PlaySfx(sfx_running);
	int framesPerCycle = PlayerWalk.numFrames() >> 2;
	int tilesPerFrame = PlayerWalk.numTilesPerFrame();
	g.sprites[0].resize(32, 32);
	for(unsigned i=0; i<48/2; ++i) {
		g.sprites[0].setImage(PlayerWalk, (i%framesPerCycle));
		g.sprites[0].move(64-16, 128-i-i);
		DoPaint();
	}
	// face front
	g.sprites[0].setImage(PlayerStand, BOTTOM);
	DoWait(0.5f);

	// look left
	g.sprites[0].setImage(PlayerIdle);
	DoWait(0.5f);
	g.sprites[0].setImage(PlayerStand, BOTTOM);
	DoWait(0.5f);

	// look right
	g.sprites[0].setImage(PlayerIdle, 1);
	DoWait(0.5f);
	g.sprites[0].setImage(PlayerStand, BOTTOM);
	DoWait(0.5f);

	// thought bubble appears
	g.bg0.image(vec(10,8), TitleThoughts);
	DoWait(0.5f);
	g.bg0.image(vec(3,4), TitleBalloon);
	DoWait(0.5f);

	// items appear
	const int pad = 36;
	const int innerPad = (128 - pad - pad) / 3;
	for(unsigned i = 0; i < 4; ++i) {
		int x = pad + innerPad * i - 8;
		g.sprites[i+1].setImage(Items, i);
		g.sprites[i+1].resize(16, 16);
		// jump
		//PlaySfx(sfx_pickup);
		for(int j=0; j<6; j++) {
			g.sprites[i+1].move(x, 42 - j);
			DoPaint();
		}
		for(int j=6; j>0; --j) {
			g.sprites[i+1].move(x, 42 - j);
			DoPaint();
		}
		g.sprites[i+1].move(x, 42);
		DoPaint();
	}
	DoWait(1.f);

	// do the pickup animation
	for(unsigned i=0; i<PlayerPickup.numFrames(); ++i) {
		g.sprites[0].setImage(PlayerPickup, i);
		DoPaint();
		DoWait(0.05f);
	}
	g.sprites[0].setImage(PlayerStand, BOTTOM);
	DoWait(2.f);

	// hide items and bubble
	g.sprites[1].hide();
	g.sprites[2].hide();
	g.sprites[3].hide();
	g.sprites[4].hide();
	g.bg0.image(vec(0,0), Sting);
	DoPaint();

	// walk off
	PlaySfx(sfx_running);
	for(unsigned i=0; i<76/2; ++i) {
		g.sprites[0].setImage(PlayerWalk, i%framesPerCycle);
		g.sprites[0].move(64-16, 80-i-i);
		DoPaint();
	}
	g.sprites[0].hide();
	//

	// slide out
	startTime = SystemTime::now();
	deadline = startTime + 1.333f;
	unsigned rowsPlotted = 0;
	while(deadline.inFuture()) {
		float u = clamp(float(SystemTime::now() - startTime) / (1.333f-0.0001f), 0.f, 1.f);
		u = 1.f - (1.f-u)*(1.f-u)*(1.f-u)*(1.f-u);
		unsigned rowsNeeded = unsigned(16.f * u)+1;
		while(rowsNeeded > rowsPlotted) {
			rowsPlotted++;
			if (rowsPlotted <= 3) {
				g.bg0.image(vec<unsigned>(0,18-rowsPlotted), vec<unsigned>(16,1), MenuFooter, vec<unsigned>(0, 3-rowsPlotted));
			} else if (rowsPlotted >= 14) {
				g.bg0.image(vec<unsigned>(0,18-rowsPlotted), vec<unsigned>(16,1), MenuHeader, vec<unsigned>(0, 16-rowsPlotted));
			} else {
				g.bg0.span(vec<unsigned>(0, 18-rowsPlotted), 16, MenuBackground);
			}
		}
		g.bg0.setPanning(vec<unsigned>(0, -int(128 * u)));
		DoPaint();
	}

	System::finish();
	g.bg0.setPanning(vec(0,0));
	g.bg0.image(vec(0,0), MenuHeader);
	g.bg0.fill(vec(0, 3), vec(16, 16-3-3), MenuBackground);
	g.bg0.image(vec(0,16-3), MenuFooter);
	DoPaint();
	System::finish();

	return &ViewAt(cube);
}
