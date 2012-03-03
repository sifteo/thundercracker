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

Game::GameState Coroutine()
{

    //TODO
    //mGame.CubeSet.LostCubeEvent += delegate { Skip(); };
    //mGame.CubeSet.NewCubeEvent += delegate { Skip(); };

    static char blankViewBuffer[NUM_CUBES][sizeof(BlankView)];

    for(int i = 0; i < NUM_CUBES; i++)
    {
        new(blankViewBuffer[i]) BlankView(&Game::cubes[i], NULL);
        Game::cubes[0].AddEventHandler(&eventHandlers[i]);

        //force a paint to initialize the screen
        Game::cubes[0].GetView()->Paint();
    }
    System::paint();

    CORO_YIELD(0.1f);
    PLAY_SFX(sfx_Stinger_02);

    for(int i = 0; i < NUM_CUBES; i++)
    {
        Game::cubes[0].OpenShuttersSync(&Title);
        ((BlankView*)Game::cubes[0].GetView())->SetImage(&Title);
        CORO_YIELD(0);
    }

    CORO_YIELD(3.0);//todo bail when skip==true

    for(int i = 0; i < NUM_CUBES; i++)
    {
        Game::cubes[0].CloseShuttersSync(&Title);
        ((BlankView*)Game::cubes[0].GetView())->SetImage(NULL);
        CORO_YIELD(0);
    }

    CORO_YIELD(0.5f);

    Game::ClearCubeEventHandlers();
    Game::ClearCubeViews();

    return Game::GameState_Menu;
}

}
}
