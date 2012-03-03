#include "sifteo.h"
#include "Game.h"

#include "StingController.h"
#include "PuzzleController.h"
#include "MenuController.h"
#include "InterstitialController.h"
#include "TutorialController.h"
#include "VictoryController.h"

namespace TotalsGame
{
    namespace Game
    {
        NeighborEventHandler *neighborEventHandler;
        
        
        
        
        float dt = 0;
        Random rand;
        
        
        void OnNeighborAdd(void*, Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
        {
            if(neighborEventHandler)
                neighborEventHandler->OnNeighborAdd(c0, s0, c1, s1);
        }
        
        void OnNeighborRemove(void*, Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
        {
            {
                if(neighborEventHandler)
                    neighborEventHandler->OnNeighborRemove(c0, s0, c1, s1);
            }
            
        }
        
        void OnCubeTouch(void*, _SYSCubeID cid)
        {
            TotalsCube *c = &Game::cubes[cid];
            c->DispatchOnCubeTouch(c, c->touching());
        }
        
        void OnCubeShake(void*, _SYSCubeID cid)
        {
            TotalsCube *cube = &Game::cubes[cid];
            cube->DispatchOnCubeShake(cube);
        }
        
        void ClearCubeViews()
        {
            for(int i = 0; i < NUM_CUBES; i++)
            {
                //cubes[0]->SetView(NULL);
                Game::cubes[i].GetView()->SetCube(NULL);
            }
        }
        
        void ClearCubeEventHandlers()
        {
            for(int i = 0; i < NUM_CUBES; i++)
            {
                cubes[0].ResetEventHandlers();
            }
        }
        
        void DrawVaultDoorsClosed()
        {
            for(int i = 0; i < NUM_CUBES; i++)
            {
                cubes[i].DrawVaultDoorsClosed();
            }
        }
        
        void PaintCubeViews()
        {
            for(int i = 0; i < NUM_CUBES; i++)
            {
                TotalsCube *c = &cubes[0];
                if(c && !c->IsTextOverlayEnabled())
                {
                    View *v = c->GetView();
                    if(v)
                        v->Paint();
                }
            }
        }
        
        void UpdateCubeViews()
        {
            for(int i = 0; i < NUM_CUBES; i++)
            {
                
                View *v = cubes[i].GetView();
                if(v)
                    v->Update();
                
            }       
        }
        
        void Run(TotalsCube *_cubes, int nCubes)
        {
            cubes = _cubes;
            
            //loading assets resets video mode to bg0 only.
            //reset to bg_spr_bg1 as needed
            for(int i = 0; i < NUM_CUBES; i++)
            {
                cubes[i].backgroundLayer.set();
            }
            
            _SYS_setVector(_SYS_NEIGHBOR_ADD , (void*)&OnNeighborAdd, NULL);
            _SYS_setVector(_SYS_NEIGHBOR_REMOVE , (void*)&OnNeighborRemove, NULL);
            _SYS_setVector(_SYS_CUBE_TOUCH, (void*)&OnCubeTouch, NULL);
            _SYS_setVector(_SYS_CUBE_SHAKE, (void*)&OnCubeShake, NULL);
            
            neighborEventHandler = NULL;
            
            currentPuzzle = NULL;
            previousPuzzle = NULL;
            
            difficulty = DifficultyHard;
            mode = NumericModeFraction;
            
            mTime = System::clock();
            
            //TODO		saveData.Load();
            
            GameState nextState = GameState_Sting;
            
            while(1)
            {
                switch(nextState)
                {
                    case GameState_Sting:
                        nextState = StingController::Coroutine();
                        break;
                        
                    case GameState_Init:
                        Initialize();
                        
                    case GameState_Puzzle:
                        nextState = PuzzleController::Coroutine();
                        break;
                        
                    case GameState_Menu:
                        nextState = MenuController::Coroutine();
                        break;
                        
                    case GameState_Interstitial:
                        nextState = InterstitialController::Coroutine();
                        break;
                        
                    case GameState_Tutorial:
                        nextState = TutorialController::Coroutine();
                        break;
                        
                    case GameState_Victory:
#if !DISABLE_CHAPTERS
                        nextState = VictoryController::Coroutine();
#endif
                        break;
                        
                    case GameState_IsOver:
                        nextState = IsGameOver();
                        break;
                        
                    case GameState_Advance:
                        nextState = Advance();
                        break;                    
                }
            }
            /*
             .Transition("sting", "Next", "init")
             .Transition("init", "NewPlayer", "tutorial")
             .Transition("init", "ReturningPlayer", "menu")
             .Transition("menu", "Tutorial", "tutorial")
             .Transition("menu", "Play", "interstitial")
             .Transition("tutorial", "Next", "interstitial")
             .Transition("interstitial", "Next", "puzzle")
             .Transition("puzzle", "Quit", "menu")
             .Transition("puzzle", "Complete", "advance")
             .Transition("advance", "RandComplete", "interstitial")//TODO"menu")
             .Transition("advance", "GameComplete", "victory")
             .Transition("advance", "NextPuzzle", "puzzle")
             .Transition("advance", "NextChapter", "victory")
             .Transition("victory", "Next", "isover")
             .Transition("isover", "Yes", "menu")
             .Transition("isover", "No", "interstitial")
             
             .SetState("sting");
             */
        }
        
        void UpdateDt()
        {
            float time = System::clock();
            dt = time - mTime;
            mTime = time;
        }
        
        void Tick()
        {
            UpdateCubeViews();
            PaintCubeViews();
            UpdateDt();
        }
        
        bool IsPlayingRandom() 
        { 
            return currentPuzzle == NULL; 
        }
        
        GameState Initialize() 
        {/* TODO
          return saveData.hasDoneTutorial ?
          "ReturningPlayer" : "NewPlayer";
          */
            return GameState_Menu;
        }
        
        GameState Advance()
        {
            delete previousPuzzle;
            previousPuzzle = currentPuzzle;
#if !DISABLE_CHAPTERS
            if (Game::currentPuzzle == NULL)
#endif //!DISABLE_CHAPTERS
            { 
                return GameState_Interstitial; //todo GameState_Menu
            }
#if !DISABLE_CHAPTERS
            currentPuzzle->SaveAsSolved();
            int chapter, puzzle;
            bool success;
            success = currentPuzzle->GetNext(NUMBER_OF_CUBES, &chapter, &puzzle);
            if (success)
            {
                saveData.AddSolvedPuzzle(currentPuzzle->chapterIndex, currentPuzzle->puzzleIndex);
                delete Game::currentPuzzle;
                Game::currentPuzzle = NULL;
                return "GameComplete";
            } 
            else if (chapter == Game::currentPuzzle->chapterIndex)
            {
                delete Game::currentPuzzle;
                Game::currentPuzzle = Database::GetPuzzleInChapter(chapter, puzzle);
                return "NextPuzzle";
            }
            else 
            {
                saveData.AddSolvedChapter(Game::currentPuzzle->chapterIndex);
                delete Game::currentPuzzle;
                Game::currentPuzzle = Database::GetPuzzleInChapter(chapter, puzzle);
                return "NextChapter";
            }
#endif // !DISABLE_CHAPTERS
        }
        
        GameState IsGameOver()
        {
            return currentPuzzle == NULL ? GameState_Menu : GameState_Interstitial;
        }
        
        
    }       				
    
}
