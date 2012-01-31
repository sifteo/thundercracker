#pragma once

#include "Guid.h"
#include "Token.h"

namespace TotalsGame 
{

	class PuzzleChapter;
	class Token;
	class TokenGroup;

	enum Difficulty 
	{
		Easy = 0, Medium = 1, Hard = 2
	};

	enum NumericMode
	{
		Fraction, Decimal
	};

	class Puzzle {
	public:
		Puzzle(int tokenCount);

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
		Token *GetToken(int index);
		TokenGroup *target;
		Token *focus;

		int hintsUsed;
		bool unlimitedHints;

	private:
		static const int MAX_TOKENS = 32;
		char tokenBufferHack[];
		Token tokens[MAX_TOKENS];
		int numTokens;
	};

}

