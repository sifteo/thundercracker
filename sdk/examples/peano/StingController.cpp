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
        Game::cubes[i].Image(Skin_Default_VaultDoor, Vec2(0,0));
    }
    System::paint();

    PLAY_SFX(sfx_Stinger_02);

    for(int i = 0; i < NUM_CUBES; i++)
    {
        Game::cubes[i].OpenShuttersToReveal(Title);
    }

    void *oldTouch = _SYS_getVectorHandler(_SYS_CUBE_TOUCH);
    void *oldShake = _SYS_getVectorHandler(_SYS_CUBE_SHAKE);

    _SYS_setVector(_SYS_CUBE_TOUCH, (void*)&OnCubeTouch, NULL);
    _SYS_setVector(_SYS_CUBE_SHAKE, (void*)&OnCubeShake, NULL);

    gotTouchOn = false;
    skip = false;

    SystemTime t = SystemTime::now() + 3.0f;
    while(!skip && t > SystemTime::now())
    {
        System::paint();
    }

    _SYS_setVector(_SYS_CUBE_TOUCH, oldTouch, NULL);
    _SYS_setVector(_SYS_CUBE_SHAKE, oldShake, NULL);

    for(int i = 0; i < NUM_CUBES; i++)
    {
        Game::cubes[i].CloseShutters();
    }

    Game::Wait(0.5f);
}

}
}
