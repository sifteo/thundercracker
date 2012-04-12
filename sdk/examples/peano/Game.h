#include "config.h"

#pragma once

#include "sifteo.h"
#include "PuzzleDatabase.h"
#include "SaveData.h"
#include "TotalsCube.h"
#include "Puzzle.h"

using namespace Sifteo;

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

        enum
        {
            RandomPuzzlesPerChapter = 5
        };


        enum GameState
        {
            GameState_Menu,
            GameState_Tutorial,
            GameState_Puzzle,
            GameState_Advance,
            GameState_Interstitial,
            GameState_Victory,
            GameState_IsOver
        };

        enum SkillLevel {
            SkillLevel_Novice,
            SkillLevel_Expert
        };

        enum ExperienceLevel {
            ExperienceLevel_Neophyte,
            ExperienceLevel_Master
        };

        extern SkillLevel skillLevel;
        ExperienceLevel GetExperienceLevel();
        Difficulty GetDifficulty();

        void SaveOptions();

		void ClearCubeViews();
		void ClearCubeEventHandlers();
		void DrawVaultDoorsClosed();
		void PaintCubeViews();

        extern TotalsCube cubes[NUM_CUBES];

        extern Puzzle *currentPuzzle;
        extern Puzzle *previousPuzzle;

		extern Random rand;

        extern int randomPuzzleCount;
		extern SaveData saveData;
		//Jukebox jukebox = new Jukebox();		

        extern float dt;

        void Run();

        void UpdateDt();
        void Wait(float delay);

        bool IsPlayingRandom();

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

