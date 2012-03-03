#include "StingController.h"

namespace TotalsGame 
{
    namespace StingController
    {
        
        bool skip = false;
        
        void OnDispose();
        
        void EventHandler::OnCubeTouch(TotalsCube *cube, bool touching)
        {
            if(!touching)
                skip = true;
        }
        
        void EventHandler::OnCubeShake(TotalsCube *cube)
        {
            skip = true;
        }
        
        //IStateController
        
        void OnSetup ()
        {            
            //TODO
            //mGame.CubeSet.LostCubeEvent += delegate { Skip(); };
            //mGame.CubeSet.NewCubeEvent += delegate { Skip(); };
        }
                
        Game::GameState Coroutine()
        {
            
            static char blankViewBuffer[NUM_CUBES][sizeof(BlankView)];
            
            CORO_BEGIN;
            
            for( i = 0; i < NUM_CUBES; i++)
            {
                new(blankViewBuffer[i]) BlankView(&Game::cubes[i], NULL);
                Game::cubes[0].AddEventHandler(&eventHandlers[i]);
                
                //force a paint to initialize the screen
                Game::cubes[0].GetView()->Paint();
            }
            System::paint();
            
            CORO_YIELD(0.1f);
            PLAY_SFX(sfx_Stinger_02);
            
            for(i = 0; i < NUM_CUBES; i++)
            {
                Game::cubes[0].OpenShuttersSync(&Title);
                ((BlankView*)Game::cubes[0].GetView())->SetImage(&Title);
                CORO_YIELD(0);
            }
            
            CORO_YIELD(3.0);//todo bail when skip==true
            
            for(i = 0; i < NUM_CUBES; i++)
            {
                Game::cubes[0].CloseShuttersSync(&Title);
                ((BlankView*)Game::cubes[0].GetView())->SetImage(NULL);
                CORO_YIELD(0);
            }
            
            CORO_YIELD(0.5f);
            
            CORO_END;
            
            return Game::GameState_Menu;
        }
                
        
        void OnDispose ()
        {
            Game::ClearCubeEventHandlers();
            Game::ClearCubeViews();
            
        }
        
    }
}
