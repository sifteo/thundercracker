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


TotalsCube *cubes;

Puzzle *currentPuzzle;
Puzzle *previousPuzzle;

Random rand;
int randomPuzzleCount;
SkillLevel skillLevel = SkillLevel_Expert;
SaveData saveData;

float dt;
int64_t timeInt;


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
        cubes[i].SetView(NULL);
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
        TotalsCube *c = &cubes[i];
        if(c && !c->IsTextOverlayEnabled())
        {
            View *v = c->GetView();
            if(v)
                v->Paint();
        }
    }
}

ExperienceLevel GetExperienceLevel()
{
    int count = 0;
    for(int i = 0; i < Database::NumChapters(); i++)
    {
        if(saveData.IsChapterSolved(i))
        {
            count++;
        }
    }
    return count > 1 ? ExperienceLevel_Master : ExperienceLevel_Neophyte;
}

Difficulty GetDifficulty()
{
    if (skillLevel == SkillLevel_Novice)
    {
        return GetExperienceLevel() == ExperienceLevel_Neophyte ? DifficultyEasy : DifficultyMedium;
    }
    else
    {
        return GetExperienceLevel() == ExperienceLevel_Neophyte ? DifficultyMedium : DifficultyHard;
    }
}

void SaveOptions()
{
    //TODO
}

void Run(TotalsCube *_cubes, int nCubes)
{
    cubes = _cubes;

    TotalsGame::AudioPlayer::Init();

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

    timeInt = System::clockNS();

    //TODO		saveData.Load();


    StingController::Run();

    GameState nextState =
    #if SKIP_INTRO_TUTORIAL
        GameState_Menu;
    #else
        saveData.HasCompletedTutorial() ? GameState_Menu : GameState_Tutorial;
    #endif


    while(1)
    {
        switch(nextState)
        {   

        case GameState_Puzzle:
            nextState = PuzzleController::Run();
            break;

        case GameState_Menu:
            nextState = MenuController::Run();
            break;

        case GameState_Interstitial:
            nextState = InterstitialController::Run();
            break;

        case GameState_Tutorial:
            nextState = TutorialController::Run();
            break;

        case GameState_Victory:
            nextState = VictoryController::Run();
            break;

        case GameState_IsOver:
            nextState = IsGameOver();
            break;

        case GameState_Advance:
            nextState = Advance();
            break;
        }
    }
}

void UpdateDt()
{
    int64_t now = System::clockNS();
    int64_t dtInt = now - timeInt;
    timeInt = now;

    dt = dtInt / 1000000000.0f;
}

void Wait(float delay)
{    
    PaintCubeViews();
    System::paintSync();

    int64_t t=System::clockNS();

    //convert to ms before goint to ns to avoid float overflow
    int64_t delayMS = delay * 1000.0f;
    t += delayMS * 1000000;
    do {
        System::yield();
        UpdateDt();
    } while(System::clockNS() < t);
}

bool IsPlayingRandom()
{
    return currentPuzzle == NULL;
}

GameState Advance()
{
    delete previousPuzzle;
    previousPuzzle = currentPuzzle;
    if (Game::currentPuzzle == NULL)
    {
        randomPuzzleCount++;
        if(randomPuzzleCount >= RandomPuzzlesPerChapter)
        {
            return GameState_Victory;
        }
        else
        {
            return GameState_Puzzle;
        }
    }

    currentPuzzle->SaveAsSolved();
    int chapter = currentPuzzle->chapterIndex;
    int puzzle = currentPuzzle->puzzleIndex;
    bool success;
    success = currentPuzzle->GetNext(NUM_CUBES, &chapter, &puzzle);
    if (!success)
    {
        saveData.AddSolvedPuzzle(currentPuzzle->chapterIndex, currentPuzzle->puzzleIndex);
        delete Game::currentPuzzle;
        Game::currentPuzzle = NULL;
        return GameState_Victory;
    }
    else if (chapter == Game::currentPuzzle->chapterIndex)
    {
        delete Game::currentPuzzle;
        Game::currentPuzzle = Database::GetPuzzleInChapter(chapter, puzzle);
        return GameState_Puzzle;
    }
    else
    {
        saveData.AddSolvedChapter(Game::currentPuzzle->chapterIndex);
        delete Game::currentPuzzle;
        Game::currentPuzzle = Database::GetPuzzleInChapter(chapter, puzzle);
        return GameState_Victory;
    }
}

GameState IsGameOver()
{
    return currentPuzzle == NULL ? GameState_Menu : GameState_Interstitial;
}


}

}
