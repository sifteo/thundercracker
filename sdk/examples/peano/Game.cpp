#include "sifteo.h"
#include "Game.h"

#include "StingController.h"
#include "PuzzleController.h"

namespace TotalsGame
{

	Random Game::rand;

	Game &Game::GetInstance()
	{
		static Game instance;
		return instance;
	}

	void Game::OnCubeTouched(_SYSCubeID cid)
	{printf("touch event\n");
		TotalsCube::EventHandler *h = Game::GetInstance().cubes[cid].eventHandler;
		if(h)
			h->OnCubeTouched();
	}

	void Game::OnCubeShake(_SYSCubeID cid)
	{
		TotalsCube::EventHandler *h = Game::GetInstance().cubes[cid].eventHandler;
		if(h)
			h->OnCubeShake();
	}

	void Game::ClearCubeViews()
	{
		for(int i = 0; i < Game::NUMBER_OF_CUBES; i++)
		{
			Game::GetInstance().cubes[i].SetView(NULL);
		}
	}

	void Game::ClearCubeEventHandlers()
	{
		for(int i = 0; i < Game::NUMBER_OF_CUBES; i++)
		{
			Game::GetInstance().cubes[i].eventHandler = NULL;
		}
	}

	void Game::Setup(TotalsCube *_cubes, int nCubes)
	{
		assert(nCubes == Game::NUMBER_OF_CUBES);
		cubes = _cubes;

		_SYS_vectors.cubeEvents.touch = &OnCubeTouched;
		_SYS_vectors.cubeEvents.shake = &OnCubeShake;

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

		sceneMgr
			.State("sting", &stingController)                //
			.State("init", &Game::Initialize)
/*			.State("menu", &menuController)                  //
			.State("tutorial", new TutorialController(this))          //
			.State("interstitial", new InterstitialController(this))  //
*/			.State("puzzle", &puzzleController)
/*			.State("advance", Advance)
			.State("victory", new VictoryController(this))            //
			.State("isover", IsGameOver)
*/
			.Transition("sting", "Next", "init")
			.Transition("init", "NewPlayer", "tutorial")
			.Transition("init", "ReturningPlayer", "puzzle")//TODO"menu")
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
		sceneMgr.Tick(dt);
		sceneMgr.Paint(true);
		mDirty = false;
	}

	void Game::CoroutineYield()
	{
		System::paint();
		float time = System::clock();
		dt = time - mTime;
		mTime = time;
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

	const char *Game::Advance(const char *transitionId)
	{
		previousPuzzle = currentPuzzle;
		if (currentPuzzle == NULL)
		{ 
			return "RandComplete";
		}
		currentPuzzle->SaveAsSolved();
		Puzzle *nextPuzzle = currentPuzzle->GetNext(NUMBER_OF_CUBES);
		if (nextPuzzle == NULL)
		{
			currentPuzzle->chapter->SaveAsSolved();
			currentPuzzle = NULL;
			return "GameComplete";
		} 
		else if (nextPuzzle->chapter == currentPuzzle->chapter)
		{
			currentPuzzle = nextPuzzle;
			return "NextPuzzle";
		}
		else 
		{
			currentPuzzle->chapter->SaveAsSolved();
			currentPuzzle = nextPuzzle;
			return "NextChapter";
		}
	}

	const char *Game::IsGameOver(const char *transitionId)
	{
		return currentPuzzle == NULL ? "Yes" : "No";
	}


}