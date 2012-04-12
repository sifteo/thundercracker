#include <sifteo.h>
#include "assets.gen.h"

#include "Game.h"

#include "config.h"

using namespace Sifteo;

void *operator new(size_t) throw() {return NULL;}
void operator delete(void*) throw() {}

void main() {
    TotalsGame::TotalsCube *cubes = TotalsGame::Game::cubes;

  for (int i = 0; i < NUM_CUBES; i++) {
    cubes[i].enable(i);
#if LOAD_ASSETS
    cubes[i].loadAssets(GameAssets);
#endif
    VidMode_BG0_ROM rom(cubes[i].vbuf);
    rom.init();
    rom.BG0_text(vec(1,1), "Loading...");
  }
#if LOAD_ASSETS
  for (;;) {
    bool done = true;
    for (int i = 0; i < NUM_CUBES; i++) {
      VidMode_BG0_ROM rom(cubes[i].vbuf);
      rom.BG0_progressBar(vec(0,7), cubes[i].assetProgress(GameAssets, VidMode_BG0::LCD_width), 2);
      done &= cubes[i].assetDone(GameAssets);
    }
    System::paint();
    if (done) break;
  }

#endif
    
    TotalsGame::Game::Run();
}


