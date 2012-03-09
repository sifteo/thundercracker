#include "StingController.h"

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

    BlankView blankViews[NUM_CUBES];

    for(int i = 0; i < NUM_CUBES; i++)
    {
        blankViews[i].SetCube(&Game::cubes[i]);
        Game::cubes[i].AddEventHandler(&eventHandlers[i]);
    }

    Game::Wait(0.1f);
    PLAY_SFX(sfx_Stinger_02);

    for(int i = 0; i < NUM_CUBES; i++)
    {
        Game::cubes[i].OpenShuttersSync(&Title);
        blankViews[i].assetImage = &Title;
        Game::Wait(0);
    }

    float t = System::clock();
    while(!skip && t+3 > System::clock())
    {
        System::paint();
    }

    for(int i = 0; i < NUM_CUBES; i++)
    {
        Game::cubes[i].CloseShuttersSync(&Title);
        blankViews[i].assetImage = NULL;
        Game::Wait(0);
    }

    Game::Wait(0.5f);

    Game::ClearCubeEventHandlers();
    Game::ClearCubeViews();

    return Game::GameState_Init;
}

}
}
