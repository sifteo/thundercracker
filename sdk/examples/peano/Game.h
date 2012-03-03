#include "config.h"

#pragma once

#include "sifteo.h"
#include "PuzzleDatabase.h"
#include "StateMachine.h"
#include "SaveData.h"
#include "TotalsCube.h"
#include "Puzzle.h"

namespace TotalsGame {

    class Puzzle;

    
	namespace Game
	{

        class NeighborEventHandler
        {
        public:
            virtual void OnNeighborAdd(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1) {};
            virtual void OnNeighborRemove(Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1) {};
        };
        
        extern NeighborEventHandler *neighborEventHandler;



        enum GameState
        {
            GameState_Sting,
            GameState_Init,
            GameState_Menu,
            GameState_Tutorial,
            GameState_Puzzle,
            GameState_Advance,
            GameState_Interstitial,
            GameState_Victory,
            GameState_IsOver
        };

		extern TotalsCube *cubes;

		void ClearCubeViews();
		void ClearCubeEventHandlers();
		void DrawVaultDoorsClosed();
		void PaintCubeViews();
        void UpdateCubeViews();

		Puzzle *currentPuzzle;
		Puzzle *previousPuzzle;

		extern Random rand;

		extern Difficulty difficulty;
		extern NumericMode mode;

		extern SaveData saveData;
		//Jukebox jukebox = new Jukebox();		

		extern float mTime;
		extern float dt;

		void Run(TotalsCube *cubes, int nCubes);

        void UpdateDt();
		void Tick();

        bool IsPlayingRandom();

		GameState Initialize();

        GameState Advance();

		GameState IsGameOver();

		void OnPause();

		void OnUnpause();


		void OnStopped();



        void OnNeighborAdd(void*, Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1);
        void OnNeighborRemove(void*, Cube::ID c0, Cube::Side s0, Cube::ID c1, Cube::Side s1);

        void OnCubeTouch(void*, _SYSCubeID cid);
        void OnCubeShake(void*, _SYSCubeID cid);
	}

}

