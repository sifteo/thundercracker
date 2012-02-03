#pragma once

#include "sifteo.h"
#include "Puzzle.h"
#include "PuzzleDatabase.h"
#include "StateMachine.h"
#include "SaveData.h"

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

		void Setup();

		void Tick();

		bool IsPlayingRandom();

		const char *Initialize(const char *transitionId);

		const char *Advance(const char *transitionId);

		const char *IsGameOver(const char *transitionId);

		void OnPause();

		void OnUnpause();


		void OnStopped();

		static void Main();
	};

}

