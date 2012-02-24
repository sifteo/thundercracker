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

	Random Game::rand;

	Game &Game::GetInstance()
	{
		static Game instance;
		return instance;
	}

    void Game::OnNeighborAdd(void*, Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
    {
        if(GetInstance().neighborEventHandler)
            GetInstance().neighborEventHandler->OnNeighborAdd(c0, s0, c1, s1);
    }

    void Game::OnNeighborRemove(void*, Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
    {
        {
            if(GetInstance().neighborEventHandler)
                GetInstance().neighborEventHandler->OnNeighborRemove(c0, s0, c1, s1);
        }

    }

    void Game::OnCubeTouch(void*, _SYSCubeID cid)
	{
        TotalsCube *c = GetCube(cid);
        c->DispatchOnCubeTouch(c, c->touching());
	}

    void Game::OnCubeShake(void*, _SYSCubeID cid)
	{
        TotalsCube *cube = GetCube(cid);
        cube->DispatchOnCubeShake(cube);
	}

	void Game::ClearCubeViews()
	{
		for(int i = 0; i < Game::NUMBER_OF_CUBES; i++)
		{
            //GetCube(i)->SetView(NULL);
            GetCube(i)->GetView()->SetCube(NULL);
		}
	}

	void Game::ClearCubeEventHandlers()
	{
		for(int i = 0; i < Game::NUMBER_OF_CUBES; i++)
		{
            GetCube(i)->ResetEventHandlers();
		}
	}

	void Game::DrawVaultDoorsClosed()
	{
		for(int i = 0; i < Game::NUMBER_OF_CUBES; i++)
		{
			Game::GetInstance().cubes[i].DrawVaultDoorsClosed();
		}
	}

	void Game::PaintCubeViews()
    {
		for(int i = 0; i < NUMBER_OF_CUBES; i++)
        {
            TotalsCube *c = Game::GetCube(i);
            if(c && !c->IsTextOverlayEnabled())
			{
				View *v = c->GetView();
				if(v)
					v->Paint();
            }
		}
	}

    void Game::UpdateCubeViews(float dt)
    {
        for(int i = 0; i < NUMBER_OF_CUBES; i++)
        {
            TotalsCube *c = Game::GetCube(i);
            if(c)
            {
                View *v = c->GetView();
                if(v)
                    v->Update(dt);
            }
        }       
    }

	void Game::Setup(TotalsCube *_cubes, int nCubes)
	{
        ASSERT(nCubes == Game::NUMBER_OF_CUBES);
		cubes = _cubes;

        _SYS_setVector(_SYS_NEIGHBOR_ADD , (void*)&OnNeighborAdd, NULL);
        _SYS_setVector(_SYS_NEIGHBOR_REMOVE , (void*)&OnNeighborRemove, NULL);
        _SYS_setVector(_SYS_CUBE_TOUCH, (void*)&OnCubeTouch, NULL);
        _SYS_setVector(_SYS_CUBE_SHAKE, (void*)&OnCubeShake, NULL);

        neighborEventHandler = NULL;

		currentPuzzle = NULL;
		previousPuzzle = NULL;

		mDirty = false;
		IsPaused = false;
        difficulty = DifficultyHard;
		mode = NumericModeFraction;

/* TODO
		var assem = System.Reflection.Assembly.GetExecutingAssembly();
		Directory.SetCurrentDirectory(Path.GetDirectoryName(assem.Location));
		while (!Directory.Exists(Path.Combine(Directory.GetCurrentDirectory(), "assets")))
		{
			Directory.SetCurrentDirectory("../");
		}
		database = PuzzleDatabase.CreateFromFile("assets/data/puzzles.xml");
		saveData = new SaveData(database);
*/
		dt = 1.0f / FrameRate;
		mTime = System::clock();

//TODO		saveData.Load();

		static StingController stingController(this);
		static PuzzleController puzzleController(this);
        static MenuController menuController(this);
        static InterstitialController interstitialController(this);
        static TutorialController tutorialController(this);
        static VictoryController victoryController(this);

		sceneMgr
			.State("sting", &stingController)                //
			.State("init", &Game::Initialize)
            .State("menu", &menuController)
            .State("tutorial", &tutorialController)          //
            .State("interstitial", &interstitialController)  //
            .State("puzzle", &puzzleController)
            .State("advance", &Game::Advance)
            .State("victory", &victoryController)            //
			.State("isover", &IsGameOver)

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
    }

	void Game::Tick()
	{
/*		if (!IsIdle)
		{ 
			return;
		} */

		float time = System::clock();
		dt = time - mTime;
		mTime = time;

        for(int i = 0; i < NUMBER_OF_CUBES; i++)
        {
            if(!cubes[i].IsTextOverlayEnabled())
            {
                cubes[i].backgroundLayer.set();
                cubes[i].backgroundLayer.setWindow(0,128);
                cubes[i].foregroundLayer.Clear();
                for(int s = 0 ; s < _SYS_VRAM_SPRITES; s++)
                    cubes[i].backgroundLayer.hideSprite(s);
            }
        }

        sceneMgr.Tick(dt);
        sceneMgr.Paint(true);

        for(int i = 0; i < NUMBER_OF_CUBES; i++)
        {
             if(!cubes[i].IsTextOverlayEnabled())
             {
                cubes[i].foregroundLayer.Flush();
             }
            cubes[i].UpdateTextOverlay();
        }

		mDirty = false;
	}

	bool Game::IsPlayingRandom() 
	{ 
		return currentPuzzle == NULL; 
	}

	const char *Game::Initialize() 
	{/* TODO
		return saveData.hasDoneTutorial ?
			"ReturningPlayer" : "NewPlayer";
			*/
		return "ReturningPlayer";
	}

    const char *Game::Advance()
	{
        Game &g = Game::GetInstance();
        delete g.previousPuzzle;
        g.previousPuzzle = g.currentPuzzle;
        if (g.currentPuzzle == NULL)
		{ 
			return "RandComplete";
		}
        g.currentPuzzle->SaveAsSolved();
        int chapter, puzzle;
        bool success;
        success = g.currentPuzzle->GetNext(NUMBER_OF_CUBES, &chapter, &puzzle);
        if (success)
		{
            Database::SavePuzzleAsSolved(g.currentPuzzle->chapterIndex, g.currentPuzzle->puzzleIndex);
            delete g.currentPuzzle;
            g.currentPuzzle = NULL;
			return "GameComplete";
		} 
        else if (chapter == g.currentPuzzle->chapterIndex)
		{
            delete g.currentPuzzle;
            g.currentPuzzle = Database::GetPuzzleInChapter(chapter, puzzle);
			return "NextPuzzle";
		}
		else 
		{
            Database::SaveChapterAsSolved(g.currentPuzzle->chapterIndex);
            delete g.currentPuzzle;
            g.currentPuzzle = Database::GetPuzzleInChapter(chapter, puzzle);
			return "NextChapter";
		}
	}

	const char *Game::IsGameOver()
	{
		return Game::GetInstance().currentPuzzle == NULL ? "Yes" : "No";
	}


}
