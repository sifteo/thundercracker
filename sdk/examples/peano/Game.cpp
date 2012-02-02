#include "sifteo.h"
#include "Game.h"

namespace TotalsGame
{

	Game &Game::GetInstance()
	{
		static Game instance;
		return instance;
	}


	void Game::Setup()
	{
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

		sceneMgr
/*			.State("sting", new StingController(this))                //
			.State("init", Initialize)
			.State("menu", new MenuController(this))                  //
			.State("tutorial", new TutorialController(this))          //
			.State("interstitial", new InterstitialController(this))  //
*/		//	.State("puzzle", new PuzzleController(this))
/*			.State("advance", Advance)
			.State("victory", new VictoryController(this))            //
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
		sceneMgr.Tick(dt);
		sceneMgr.Paint(mDirty);
		mDirty = false;
	}

	bool Game::IsPlayingRandom() 
	{ 
		return currentPuzzle == NULL; 
	}

	const char *Game::Initialize(const char *transitionId) 
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
		var nextPuzzle = currentPuzzle->GetNext(CubeSet.Count);
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

	void Game::OnPause() 
	{
		IsPaused = true;
		mTimer.Stop();
		foreach(var cube in CubeSet) 
		{
			cube.Image("pause");
			cube.Paint();
		}
	}

	void Game::OnUnpause() 
	{
		mTimer.Start();
		IsPaused = false;
		mDirty = true;
	}


	void Game::OnStopped()
	{
		mTimer.Stop();
		Inst = null;
	}

	void Game::Main() 
	{
		GetInstance().Setup();
		while(1)
		{
			GetInstance().Tick();
		}
	}
}