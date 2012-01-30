#include "Guid.h"

namespace TotalsGame {
	
	enum Difficulty {
		Easy = 0, Medium = 1, Hard = 2
	};
	
	enum NumericMode {
		Fraction, Decimal
	};
	
	enum Op {
		Add = 0,
		Subtract = 1,
		Multiply = 2,
		Divide = 3
	};
	
	class Puzzle {
  public:

    // option parameters
    void *userData;
    Guid guid;
    PuzzleChapter *chapter;
    Difficulty difficulty;

    // game parameters
    Token *GetToken(int index) {assert(index < numTokens); return tokens[index];}
    TokenGroup *target;
    Token *focus;

	Puzzle()
	{
		userData = NULL;
		guid = Guid::Empty;
		chapter = NULL;
		difficulty = Difficulty::Hard;
		focus = NULL;
		target = NULL;
		focus = NULL;
	}

    int hintsUsed = 0;
    bool unlimitedHints = false;

		Puzzle(int tokenCount) {
			assert(tokenCount <= MAX_TOKENS);
			numTokens = tokenCount;
			for (int i=0; i<tokenCount; ++i) {
				new(tokens+i) Token(this, i);
			}
		}

    void ClearUserdata() {
      for(int i=0; i<numTokens; ++i) {
        tokens[i].current = tokens[i];
        tokens[i].userData = null;
      }
    }

    void ClearGroups() {
      for(int i=0; i<numTokens; ++i) {
        tokens[i].current = tokens[i];
      }
    }
		
		bool IsComplete() {
			if (target == null) { return false; }
			var soln = tokens[0].current;
			// have we hit the target value?
			if (!soln.Value.Equals(target.Value)) { return false; }
			// are all the other tokens participating?
			for(int i=1; i<numTokens; ++i) {
			  if (tokens[i].current != soln) {
			    return false;
			  }
			}
			return true;
		}

    Difficulty GetDifficulty() {
      return Game.Inst == null ? difficulty : Game.Inst.difficulty;
    }
		
	private:
			static const int MAX_TOKENS = 32;
			Token tokens[MAX_TOKENS];
			int numTokens;
	}
  
}

