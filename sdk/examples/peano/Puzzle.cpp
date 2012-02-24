#include "TokenGroup.h"
#include "Guid.h"
#include "Game.h"
#include "Puzzle.h"
#include "PuzzleHelper.h"
#include "PuzzleDatabase.h"

namespace TotalsGame {
		
	DEFINE_POOL(Puzzle)

    Puzzle::Puzzle(int tokenCount, int chapter, int puzzle)
	{
        chapterIndex = chapter;
        puzzleIndex = puzzle;
		difficulty = DifficultyEasy;
		focus = NULL;
		target = NULL;
		focus = NULL;
		hintsUsed = 0;
		unlimitedHints = false;

        ASSERT(tokenCount <= MAX_TOKENS);
		numTokens = tokenCount;
		for (int i=0; i<tokenCount; ++i) {
			tokens[i] = new Token(this, i);
		}
	}

    Puzzle::~Puzzle()
    {
        //clean up the tokens and token groups
        // groups contained by token tree and also target
        Token::ResetAllocationPool();
        TokenGroup::ResetAllocationPool();

   /*
        numTokens = tokenCount;
        for (int i=0; i<tokenCount; ++i) {
            delete tokens[i];
*/
    }

    int Puzzle::GetNumTokens()
    {
        return numTokens;
    }
    
	Token *Puzzle::Puzzle::GetToken(int index)
	{
        ASSERT(index < numTokens);
		return tokens[index];
	}

	void Puzzle::ClearUserdata()
	{
		for(int i=0; i<numTokens; ++i)
		{
			tokens[i]->current = tokens[i];
            tokens[i]->view = NULL;
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
        Database::SavePuzzleAsSolved(chapterIndex, puzzleIndex);
        Game::GetInstance().saveData.Save();
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
    
    bool Puzzle::GetNext(int *chapter, int *puzzle)
    {
        if (chapterIndex == -1) { *chapter = *puzzle = -1; return false; }
        if (puzzleIndex < Database::NumPuzzlesInChapter(chapterIndex)-1) {
            *chapter = chapterIndex;
            *puzzle = puzzleIndex + 1;
            return true;
        }
        if (chapterIndex == -1) { *chapter = *puzzle = -1; return false; }
        if (chapterIndex < Database::NumChapters() - 1) {
            *chapter = chapterIndex + 1;
            *puzzle = 0;
            return true;
        }
        *chapter = *puzzle = -1;
        return false;
    }
    
    bool Puzzle::GetNext(int maxCubeCount, int *chapter, int *puzzle)
    {
        bool success;
        do {
            success = GetNext(chapter, puzzle);
        } while(success && Database::NumTokensInPuzzle(*chapter, *puzzle) > maxCubeCount);
        return success;
    }

    int Puzzle::CountAfterThisInChapterWithCurrentCubeSet()
    {
      if (chapterIndex == -1) { return 0; }
      int result = 0;
      for(int i=puzzleIndex+1; i<Database::NumPuzzlesInChapter(chapterIndex); ++i)
      {
        if (Database::NumTokensInPuzzle(chapterIndex, i) <= Game::NUMBER_OF_CUBES)
        {
          result++;
        }
      }
      return result;
    }

	bool Puzzle::SelectRandomTarget() 
	{		
		IExpression *expressions[6];
		int numExpressions = 0;

		for(int i = 0; i < numTokens; i++)
		{
			expressions[numExpressions++] = (tokens[i]);
		}

		//mix up the epxressions
		for(int i = 0; i < numExpressions; i++)
		{
			int a = Game::rand.randrange(numExpressions);
			IExpression *temp = expressions[i];
			expressions[i] = expressions[a];
			expressions[a] = temp;
		}
		
		while(numExpressions > 1)
		{
			// remove two random expressions
			IExpression *src = expressions[--numExpressions];
			IExpression *dst = expressions[--numExpressions];
			// possibly flip
			if(Game::rand.randrange(2) == 0)
			{
				IExpression *tmp  = src;
				src = dst;
				dst = tmp;
			}
			TokenGroup *comp = PuzzleHelper::ConnectRandomly(src, dst);
			
			if (comp != NULL) 
			{
				expressions[numExpressions++] = comp;
			} 
			else if (numExpressions == 0) 
			{
				return false;
			}
		}
		target = (TokenGroup*)expressions[0];
		return true;
	}

}

