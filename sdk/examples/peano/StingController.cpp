#include "StingController.h"
#include "AudioPlayer.h"
#include "assets.gen.h"

namespace TotalsGame 
{
namespace StingController
{

class EventHandler: public TotalsCube::EventHandler
{
public:
    virtual void OnCubeTouch(TotalsCube *cube, bool touching);
    virtual void OnCubeShake(TotalsCube *cube);
};


EventHandler eventHandlers[NUM_CUBES];

bool skip = false;

void EventHandler::OnCubeTouch(TotalsCube *cube, bool touching)
{
    if(!touching)
        skip = true;
}

void EventHandler::OnCubeShake(TotalsCube *cube)
{
    skip = true;
}

Game::GameState Run()
{

    //TODO lost and found
    //mGame.CubeSet.LostCubeEvent += delegate { Skip(); };
    //mGame.CubeSet.NewCubeEvent += delegate { Skip(); };

    for(int i = 0; i < NUM_CUBES; i++)
    {
        Game::cubes[i].DrawVaultDoorsClosed();
        Game::cubes[i].AddEventHandler(&eventHandlers[i]);
    }

    Game::Wait(0.1f);
    PLAY_SFX(sfx_Stinger_02);

    for(int i = 0; i < NUM_CUBES; i++)
    {
        Game::cubes[i].OpenShuttersSync(&Title);
        Game::cubes[i].Image(&Title, Vec2(0,0));
        Game::Wait(0);
    }

    int64_t t = System::clockNS() + 3 * int64_t(1000000000);
    while(!skip && t > System::clockNS())
    {
        System::paint();
    }

    for(int i = 0; i < NUM_CUBES; i++)
    {
        Game::cubes[i].CloseShuttersSync(&Title);
        Game::Wait(0);
    }

    Game::Wait(0.5f);

    Game::ClearCubeEventHandlers();
    Game::ClearCubeViews();

    return Game::GameState_Init;
}

}
}
