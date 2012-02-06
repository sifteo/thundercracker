#pragma once

#include "sifteo.h"
#include "Puzzle.h"
#include "PuzzleDatabase.h"
#include "StateMachine.h"
#include "SaveData.h"
#include "TotalsCube.h"

namespace TotalsGame {

	class Game
	{
	private:
		Game() {};	//singleton

	public:
        static const int NUMBER_OF_CUBES = 4;		
        
		static Game &GetInstance();
		StateMachine sceneMgr;

		static const int FrameRate = 15;

		TotalsCube *cubes;

		Puzzle *currentPuzzle;
		Puzzle *previousPuzzle;

		Difficulty difficulty;
		NumericMode mode;
		Random seed;
		PuzzleDatabase database;		
		SaveData saveData;
		//Jukebox jukebox = new Jukebox();		
		bool mDirty;
		float mTime;
		float dt;
		bool IsPaused;

		void Setup(TotalsCube *cubes, int nCubes);

		void Tick();
		void CoroutineYield();
		static void Yield() {Game::GetInstance().CoroutineYield();}

		bool IsPlayingRandom();

		const char *Initialize(const char *transitionId);

		const char *Advance(const char *transitionId);

		const char *IsGameOver(const char *transitionId);

		void OnPause();

		void OnUnpause();


		void OnStopped();

	};

}

