#include <sifteo.h>
#include "assets.gen.h"

#include "Game.h"

#include "config.h"

using namespace Sifteo;


static AssetSlot MainSlot = AssetSlot::allocate();

static Metadata M = Metadata() 
.title("Peano\'s Vault") 
.cubeRange(6);



void *operator new(unsigned long) throw() {return NULL;}
void operator delete(void*) throw() {}

namespace TotalsGame 
{
    Int2 kSideToUnit[4]=
    {
        vec(0, -1),
        vec(-1, 0),
        vec(0, 1),
        vec(1, 0)
    };
}

AssetLoader loader;

void main() {
    TotalsGame::TotalsCube *cubes = TotalsGame::Game::cubes;
        
    loader.start(GameAssets, 0, (uint32_t)((1<<NUM_CUBES)-1)<<(32-NUM_CUBES));

  for (int i = 0; i < NUM_CUBES; i++) {
      cubes[i].sys = i;
      cubes[i].vid.initMode(BG0_ROM);
      cubes[i].vid.attach(cubes[i]);
      cubes[i].vid.bg0rom.text(vec(1,1), "Loading...");
  }
#if LOAD_ASSETS
  while(!loader.isComplete()) {
    for (int i = 0; i < NUM_CUBES; i++) {
      cubes[i].vid.bg0rom.hBargraph(vec(0,7), loader.progress(i, 128));
    }
    System::paint();
  }

#endif
    
    TotalsGame::Game::Run();
}


