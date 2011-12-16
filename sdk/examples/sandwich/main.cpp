#include "Game.h"

static Game sGame;
Game* pGame = &sGame;

void siftmain() {
	// TODO: Move Cubes out of GameView to separate Alloc?
	{ // initialize assets
	  for (Cube::ID i = 0; i < NUM_CUBES; i++) {
	    sGame.CubeAt(i)->enable(i);
	    sGame.CubeAt(i)->loadAssets(GameAssets);
	    VidMode_BG0_ROM rom(sGame.CubeAt(i)->vbuf);
	    rom.init();
	    rom.BG0_text(Vec2(1,1), "Loading...");
	  }
	  bool done = false;
	  while(!done) {
	  	done = true;
	    for (Cube::ID i = 0; i < NUM_CUBES; i++) {
	      VidMode_BG0_ROM rom(sGame.CubeAt(i)->vbuf);
	      rom.BG0_progressBar(Vec2(0,7), sGame.CubeAt(i)->assetProgress(GameAssets, VidMode_BG0::LCD_width), 2);
	      done &= sGame.CubeAt(i)->assetDone(GameAssets);
	    }
	    System::paint();
	  }
	  for(Cube::ID i=0; i<NUM_CUBES; ++i) {
	    VidMode_BG0 mode(sGame.CubeAt(i)->vbuf);
	    mode.init();
	    mode.BG0_drawAsset(Vec2(0,0), Sting);
	  }
	  float time = System::clock();
	  while(System::clock() - time < 1.f) {
	  	for(int i=0; i<NUM_CUBES; ++i) {
	  		sGame.CubeAt(i)->vbuf.touch();
	  	}
	    System::paint();
	  }		
	}
	sGame.MainLoop();
}
