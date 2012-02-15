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

        class NeighborEventHandler
        {
        public:
            virtual void OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1);
            virtual void OnNeighborRemove(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1);
        };
        NeighborEventHandler *neighborEventHandler;


        static const int NUMBER_OF_CUBES = 4;		
        
		static Game &GetInstance();
		StateMachine sceneMgr;

		static const int FrameRate = 15;

		TotalsCube *cubes;
		static TotalsCube *GetCube(int i) {return &Game::GetInstance().cubes[i];}
		static void ClearCubeViews();
		static void ClearCubeEventHandlers();
		static void DrawVaultDoorsClosed();
		static void PaintCubeViews();
        static void UpdateCubeViews(float dt);

		Puzzle *currentPuzzle;
		Puzzle *previousPuzzle;

		static Random rand;

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

        bool IsPlayingRandom();

		static const char *Initialize();

		const char *Advance(const char *transitionId);

		const char *IsGameOver(const char *transitionId);

		void OnPause();

		void OnUnpause();


		void OnStopped();

	private:

        static void OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1);
        static void OnNeighborRemove(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1);

		static void OnCubeTouch(_SYSCubeID cid);
		static void OnCubeShake(_SYSCubeID cid);
	};

}

