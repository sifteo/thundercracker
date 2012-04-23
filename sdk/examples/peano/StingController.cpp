#include "StingController.h"
#include "AudioPlayer.h"
#include "assets.gen.h"

namespace TotalsGame 
{
namespace StingController
{

bool gotTouchOn=false;
bool skip = false;

void OnCubeTouch(void*, unsigned cid)
{
    if(Game::cubes[cid].isTouching())
        gotTouchOn = true;
    else if(gotTouchOn) //touch off now, but got touch on before
        skip = true;
}

void OnCubeShake(void*, unsigned cid)
{
    skip = true;
}

void Run()
{

    //TODO lost and found
    //mGame.CubeSet.LostCubeEvent += delegate { Skip(); };
    //mGame.CubeSet.NewCubeEvent += delegate { Skip(); };

    for(int i = 0; i < NUM_CUBES; i++)
    {
        Game::cubes[i].DrawVaultDoorsClosed();
    }
    System::paint();

    PLAY_SFX(sfx_Stinger_02);

    for(int i = 0; i < NUM_CUBES; i++)
    {
        Game::cubes[i].OpenShuttersToReveal(Title);
    }

    void *oldTouch = Events::cubeTouch.handler();
    void *oldShake = Events::cubeShake.handler();

    Events::cubeTouch.set(&OnCubeTouch);
    Events::cubeShake.set(&OnCubeShake);

    gotTouchOn = false;
    skip = false;

    SystemTime t = SystemTime::now() + 3.0f;
    while(!skip && t > SystemTime::now())
    {
        System::paint();
    }

    Events::cubeTouch.set((void(*)(void*,unsigned ))oldTouch);
    Events::cubeShake.set((void(*)(void*,unsigned))oldShake);

    for(int i = 0; i < NUM_CUBES; i++)
    {
        Game::cubes[i].CloseShutters();
    }

    Game::Wait(0.5f);
}

}
}
