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
    void* userData = null;
    Guid guid = Guid.Empty;
    PuzzleChapter *chapter = null;
    Difficulty difficulty = Difficulty.Hard;

    // game parameters
    Token *GetToken(int index) {return tokens[indexer];}
    TokenGroup *target;
    Token *focus = null;

    public int hintsUsed = 0;
    public bool unlimitedHints = false;

		public Puzzle(int tokenCount) {
			tokens = new Token[tokenCount];
			for (int i=0; i<tokenCount; ++i) {
				tokens[i] = new Token(this, i);
			}
		}

    public void ClearUserdata() {
      for(int i=0; i<tokens.Length; ++i) {
        tokens[i].current = tokens[i];
        tokens[i].userData = null;
      }
    }

    public void ClearGroups() {
      for(int i=0; i<tokens.Length; ++i) {
        tokens[i].current = tokens[i];
      }
    }
		
		public bool IsComplete {
      get {
        if (target == null) { return false; }
        var soln = tokens[0].current;
        // have we hit the target value?
        if (!soln.Value.Equals(target.Value)) { return false; }
        // are all the other tokens participating?
        for(int i=1; i<tokens.Length; ++i) {
          if (tokens[i].current != soln) {
            return false;
          }
        }
        return true;
      }
		}

    public Difficulty Difficulty {
      get { return Game.Inst == null ? difficulty : Game.Inst.difficulty; }
    }
	}
  
}

