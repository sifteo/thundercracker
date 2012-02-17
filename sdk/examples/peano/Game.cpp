#include "sifteo.h"
#include "Game.h"

#include "StingController.h"
#include "PuzzleController.h"
#include "MenuController.h"
#include "InterstitialController.h"
#include "TutorialController.h"

namespace TotalsGame
{

	Random Game::rand;

	Game &Game::GetInstance()
	{
		static Game instance;
		return instance;
	}

    void Game::OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
    {
        if(GetInstance().neighborEventHandler)
            GetInstance().neighborEventHandler->OnNeighborAdd(c0, s0, c1, s1);
    }

    void Game::OnNeighborRemove(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1)
    {
        {
            if(GetInstance().neighborEventHandler)
                GetInstance().neighborEventHandler->OnNeighborRemove(c0, s0, c1, s1);
        }

    }

	void Game::OnCubeTouch(_SYSCubeID cid)
	{
        TotalsCube *c = GetCube(cid);
        c->DispatchOnCubeTouch(c, c->touching());
	}

	void Game::OnCubeShake(_SYSCubeID cid)
	{
        TotalsCube *cube = GetCube(cid);
        cube->DispatchOnCubeShake(cube);
	}

	void Game::ClearCubeViews()
	{
		for(int i = 0; i < Game::NUMBER_OF_CUBES; i++)
		{
            GetCube(i)->SetView(NULL);
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
			if(c)
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
		assert(nCubes == Game::NUMBER_OF_CUBES);
		cubes = _cubes;

        _SYS_vectors.neighborEvents.add = &OnNeighborAdd;
        _SYS_vectors.neighborEvents.remove = &OnNeighborRemove;
		_SYS_vectors.cubeEvents.touch = &OnCubeTouch;
		_SYS_vectors.cubeEvents.shake = &OnCubeShake;

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

		sceneMgr
			.State("sting", &stingController)                //
			.State("init", &Game::Initialize)
            .State("menu", &menuController)
            .State("tutorial", &tutorialController)          //
            .State("interstitial", &interstitialController)  //
            .State("puzzle", &puzzleController)
            .State("advance", &Game::Advance)
/*			.State("victory", new VictoryController(this))            //
			.State("isover", IsGameOver)
*/
			.Transition("sting", "Next", "init")
			.Transition("init", "NewPlayer", "tutorial")
            .Transition("init", "ReturningPlayer", "menu")
			.Transition("menu", "Tutorial", "tutorial")
			.Transition("menu", "Play", "interstitial")
			.Transition("tutorial", "Next", "interstitial")
			.Transition("interstitial", "Next", "puzzle")
			.Transition("puzzle", "Quit", "menu")
			.Transition("puzzle", "Complete", "advance")
			.Transition("advance", "RandComplete", "menu")
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
            cubes[i].foregroundLayer.Clear();
            for(int s = 0 ; s < _SYS_VRAM_SPRITES; s++)
                cubes[i].backgroundLayer.hideSprite(s);
        }

        sceneMgr.Tick(dt);
        sceneMgr.Paint(true);

        for(int i = 0; i < NUMBER_OF_CUBES; i++)
        {
            cubes[i].foregroundLayer.Flush();
        }

		mDirty = false;

        //TODO System::paintSync();
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
        g.previousPuzzle = g.currentPuzzle;
        if (g.currentPuzzle == NULL)
		{ 
			return "RandComplete";
		}
        g.currentPuzzle->SaveAsSolved();
        Puzzle *nextPuzzle = g.currentPuzzle->GetNext(NUMBER_OF_CUBES);
		if (nextPuzzle == NULL)
		{
            g.currentPuzzle->chapter->SaveAsSolved();
            g.currentPuzzle = NULL;
			return "GameComplete";
		} 
        else if (nextPuzzle->chapter == g.currentPuzzle->chapter)
		{
            g.currentPuzzle = nextPuzzle;
			return "NextPuzzle";
		}
		else 
		{
            g.currentPuzzle->chapter->SaveAsSolved();
            g.currentPuzzle = nextPuzzle;
			return "NextChapter";
		}
	}

	const char *Game::IsGameOver(const char *transitionId)
	{
		return currentPuzzle == NULL ? "Yes" : "No";
	}


}
