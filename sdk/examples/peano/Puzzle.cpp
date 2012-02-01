#include "TokenGroup.h"
#include "Guid.h"
#include "Puzzle.h"


namespace TotalsGame {
		
	Puzzle::Puzzle(int tokenCount) 
	{
		userData = NULL;
		chapter = NULL;
		difficulty = Hard;
		focus = NULL;
		target = NULL;
		focus = NULL;
		hintsUsed = 0;
		unlimitedHints = false;

		assert(tokenCount <= MAX_TOKENS);
		numTokens = tokenCount;
		for (int i=0; i<tokenCount; ++i) {
			tokens[i] = new Token(this, i);
		}
	}

	Token *Puzzle::Puzzle::GetToken(int index)
	{
		assert(index < numTokens); 
		return tokens[index];
	}

	void Puzzle::ClearUserdata()
	{
		for(int i=0; i<numTokens; ++i)
		{
			tokens[i]->current = tokens[i];
			tokens[i]->userData = NULL;
		}
	}

	void Puzzle::ClearGroups()
	{
		for(int i=0; i<numTokens; ++i)
		{
			tokens[i]->current = tokens[i];
		}
	}

	bool Puzzle::IsComplete()
	{
		if (target == NULL) 
		{ 
			return false; 
		}

		IExpression *soln = tokens[0]->current;
		// have we hit the target value?
		if (soln->GetValue() != target->GetValue()) 
		{
			return false;
		}
		// are all the other tokens participating?
		for(int i=1; i<numTokens; ++i)
		{
			if (tokens[i]->current != soln)
			{
				return false;
			}
		}
		return true;
	}

	Difficulty Puzzle::GetDifficulty()
	{
		return Game::Instance() == NULL ? difficulty : Game::Instance.difficulty;
	}


}

