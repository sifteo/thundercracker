#include "StingController.h"
#include "AudioPlayer.h"
#include "assets.gen.h"

namespace TotalsGame 
{
namespace StingController
{

    bool gotTouchOn=false;
bool skip = false;

void OnCubeTouch(void*, _SYSCubeID cid)
{
    if(Game::cubes[cid].touching())
        gotTouchOn = true;
    else if(gotTouchOn) //touch off now, but got touch on before
        skip = true;
}

void OnCubeShake(void*, _SYSCubeID cid)
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
        Game::cubes[i].Image(Skin_Default_VaultDoor);
    }
    System::paint();

    PLAY_SFX(sfx_Stinger_02);

    for(int i = 0; i < NUM_CUBES; i++)
    {
        Game::cubes[i].OpenShuttersToReveal(Title);
    }

    _SYS_setVector(_SYS_CUBE_TOUCH, (void*)&OnCubeTouch, NULL);
    _SYS_setVector(_SYS_CUBE_SHAKE, (void*)&OnCubeShake, NULL);

    int64_t t = System::clockNS() + 3 * int64_t(1000000000);
    while(!skip && t > System::clockNS())
    {
        System::paint();
    }

    _SYS_setVector(_SYS_CUBE_TOUCH, NULL, NULL);
    _SYS_setVector(_SYS_CUBE_SHAKE, NULL, NULL);

    for(int i = 0; i < NUM_CUBES; i++)
    {
        Game::cubes[i].CloseShutters();
    }

    Game::Wait(0.5f);
}

}
}
