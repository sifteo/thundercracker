#include <sifteo.h>
#include "assets.gen.h"

#include "Game.h"

#include "config.h"

using namespace Sifteo;


static AssetSlot MainSlot = AssetSlot::allocate()
        .bootstrap(GameAssets);

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

void main() {
    TotalsGame::TotalsCube *cubes = TotalsGame::Game::cubes;
   for (int i = 0; i < NUM_CUBES; i++) {
      cubes[i].sys = i;
   }

    TotalsGame::Game::Run();
}


