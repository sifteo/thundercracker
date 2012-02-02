#include "TokenGroup.h"
#include "Guid.h"
#include "Game.h"
#include "Puzzle.h"


namespace TotalsGame {
		
	DEFINE_POOL(Puzzle)

	Puzzle::Puzzle(int tokenCount) 
	{
		userData = NULL;
		chapter = NULL;
		difficulty = DifficultyHard;
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
		return Game::GetInstance().difficulty;
	}


	//from SaveDataHelper in c#
	/*
	    public static bool HasBeenSolved(this Puzzle p) {
      if (p == null || Game.Inst == null) { return false; }
      return p.guid != Guid.Empty && Game.Inst.saveData.solved.Contains(p.guid);
    }*/

	void Puzzle::SaveAsSolved() 
	{
		if (guid != Guid::Empty) 
		{
			Game::GetInstance().saveData.AddSolved(guid);
			Game::GetInstance().saveData.Save();
		}
	}
	/*
    public static int CountAfterThisInChapterWithCurrentCubeSet(this Puzzle p) {
      if (p == null || p.chapter == null || Game.Inst == null) { return 0; }
      int result = 0;
      for(int i=p.chapter.Puzzles.IndexOf(p)+1; i<p.chapter.Puzzles.Count; ++i) {
        if (p.chapter.Puzzles[i].tokens.Length <= Game.Inst.CubeSet.Count) {
          result++;
        }
      }
      return result;
    }*/

}

