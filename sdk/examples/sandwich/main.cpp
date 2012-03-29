#include "Game.h"
#include "Dialog.h"

Cube gCubes[NUM_CUBES];
#if SFX_ON
AudioChannel gChannelSfx;
#endif
#if MUSIC_ON
AudioChannel gChannelMusic;
#endif

void main() {
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
		gGame.MainLoop();
	}
}
