#pragma once

#include "Guid.h"
#include "Token.h"
#include "ObjectPool.h"

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

		DECLARE_POOL(Puzzle, 4)

	public:
		Puzzle(int tokenCount);
        ~Puzzle();

		void ClearUserdata();
		void ClearGroups();
		bool IsComplete();
		Difficulty GetDifficulty();


		// option parameters
		void *userData;
		Guid guid;
		PuzzleChapter *chapter;
		Difficulty difficulty;

		// game parameters
        int GetNumTokens();
		Token *GetToken(int index);
		TokenGroup *target;
		Token *focus;

		int hintsUsed;
		bool unlimitedHints;

		void SaveAsSolved();
        
        Puzzle *GetNext();
        Puzzle *GetNext(int maxCubeCount);
        int CountAfterThisInChapterWithCurrentCubeSet();

		bool SelectRandomTarget();

	private:
		static const int MAX_TOKENS = 32;
		Token *tokens[MAX_TOKENS];
		int numTokens;
	};

}

