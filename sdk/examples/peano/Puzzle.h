#pragma once

//#include "Guid.h"
#include "ObjectPool.h"
#include <stddef.h>

namespace TotalsGame 
{
	class PuzzleChapter;
	class Token;
	class TokenGroup;

	enum Difficulty 
	{
		DifficultyEasy = 0, DifficultyMedium = 1, DifficultyHard = 2
	};

	enum NumericMode
	{
		NumericModeFraction, NumericModeDecimal
	};

	class Puzzle {

        DECLARE_POOL(Puzzle, 2)

	public:
        Puzzle(int tokenCount, int chapter=-1, int puzzle=-1);
        ~Puzzle();

		void ClearUserdata();
		void ClearGroups();
		bool IsComplete();
		Difficulty GetDifficulty();


		// option parameters
		//Guid guid;
        int puzzleIndex, chapterIndex;
		Difficulty difficulty;

		// game parameters
        int GetNumTokens();
		Token *GetToken(int index);
		TokenGroup *target;
		Token *focus;

		int hintsUsed;
		bool unlimitedHints;

		void SaveAsSolved();
        
        bool GetNext(int *chapter, int *puzzle);
        bool GetNext(int maxCubeCount, int *chapter, int *puzzle);
        int CountAfterThisInChapterWithCurrentCubeSet();

		bool SelectRandomTarget();

	private:
		static const int MAX_TOKENS = 32;
		Token *tokens[MAX_TOKENS];
		int numTokens;
	};

}

