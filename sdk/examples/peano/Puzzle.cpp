#include "TokenGroup.h"
#include "Guid.h"
#include "Game.h"
#include "Puzzle.h"
#include "PuzzleChapter.h"
#include "PuzzleHelper.h"


namespace TotalsGame {
		
	DEFINE_POOL(Puzzle)

	Puzzle::Puzzle(int tokenCount) 
	{
		chapter = NULL;
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
    
    Puzzle *Puzzle::GetNext()
    {
        if (chapter == NULL) { return NULL; }
        int localIndex = chapter->IndexOfPuzzle(this);
        if (localIndex < chapter->NumPuzzles()-1) {
            return chapter->GetPuzzle(localIndex+1);
        }
        if (chapter->db == NULL) { return NULL; }
        int globalIndex = chapter->db->IndexOfChapter(chapter);
        if (globalIndex < chapter->db->NumChapters()-1) {
            return chapter->db->GetChapter(globalIndex+1)->GetPuzzle(0);
        }
        return NULL;
    }
    
    Puzzle *Puzzle::GetNext(int maxCubeCount) {
        Puzzle *puzzle = this;
        do {
            puzzle = puzzle->GetNext();
        } while(puzzle != NULL && puzzle->numTokens > maxCubeCount);
        return puzzle;
    }

    int Puzzle::CountAfterThisInChapterWithCurrentCubeSet()
    {
      if (chapter == NULL) { return 0; }
      int result = 0;
      for(int i=chapter->IndexOfPuzzle(this)+1; i<chapter->NumPuzzles(); ++i)
      {
        if (chapter->GetPuzzle(i)->GetNumTokens() <= Game::NUMBER_OF_CUBES)
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

