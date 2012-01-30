#include "IExpression.h"

namespace TotalsGame {
	
	enum SideStatus {
    Open,       // physically no tokens are on this side
    Connected,  // this side is participating in the solution
    Blocked     // this side is NOT participating in the solution, but it physically blocked
  };
 
	class Token : public IExpression {
  public:

    Puzzle *GetPuzzle() {return puzzle;}
    int GetIndex() {return indexer;}
		Op opRight[] = { Op.Add, Op.Add, Op.Add };
		Op opBottom[] = { Op.Subtract, Op.Subtract, Op.Subtract };
		int val = 0;
    IExpression *current;
       
    void *userData = null;

		Token(Puzzle *p, int i) {
      // instantiated by puzzle's constructor
			puzzle = p;
      index = i;
      current = this;
		}

    // IExpression impl

    virtual Fraction GetValue {return val;}
    virtual int GetDepth {return -1;}
    virtual int GetCount {return 1;}
    virtual ShapeMask GetMask {return ShapeMask.Unity;}

    virtual bool TokenAt(const Int2 &p, Token **t) {
      if (p.x == 0 && p.y == 0) {
        *t = this;
        return true;
      }
      *t = null;
      return false;
    }

    virtual bool PositionOf(Token *t, Int2 *p) {
      *p = Vec2.Zero;
      return t == this;
    }

    virtual bool Contains(Token *t) {
      return t == this;
    }

    virtual void SetCurrent(IExpression *exp) {
      current = exp;
    }

    // getters
    
    Fraction GetCurrentTotal() {
      if (puzzle->target == null) {
        return current->GetValue();
       }
       return current == this ? puzzle->target->GetValue() : current->GetValue();
      }
    }

    void PopGroup() {
      if (current->ContainsSubExpressions())
	  {
		  current = current->GetSubExpressionContaining(this);
	  }
    }

    SideStatus StatusOfSide(Cube::Side side, IExpression *current) {
      if (current == this) { return SideStatus.Open; }
      Vec2 pos;
      current->PositionOf(this, &pos);
      Vec2 del = Vec2.Side(side);
      if (!current->Mask.BitAt(pos+del)) {
        return SideStatus::Open;
      }
      IExpression *grp = current;
      if (side->IsSource()) {
        while(grp->ContainsSubExpressions()) {
          if (grp->srcToken == this && grp->srcSide == side) { return SideStatus::Connected; }
          grp = grp->GetSubExpressionContaining(this);
        }
      } else {
        side = CubeHelper::InvertSide(side);
        while(grp->ContainsSubExpressions()) {
          if (grp->dstToken == this && grp->srcSide == side) { return SideStatus::Connected; }
          grp = grp->GetSubExpressionContaining(this);
        }
      }
      return SideStatus::Blocked;
    }

    bool ConnectsOnSideAtDepth(Cube::Side s, int depth, IExpression *exp) {
      if (exp == null) {
        exp = current;
      }
      IExpression *grp = exp;
      if (s->IsSource()) {
        while(grp.ContainsSubExpressions()) {
          if (grp->srcToken == this && grp->srcSide == s && depth >= grp->GetDepth() && CheckDepth(depth, exp)) { return true; }
          grp = grp->GetSubExpressionContaining(this);
        }
      } else {
        s = CubeHelper::InvertSide(s);
        while(grp->ContainsSubExpressions()) {
          if (grp->dstToken == this && grp->srcSide == s && depth >= grp->GetDepth() && CheckDepth(depth, exp)) { return true; }
          grp = grp->GetSubExpressionContaining(this);
        }
      }
      return false;
    }

    bool CheckDepth(int depth, IExpression *exp) {
      IExpression *grp = exp;
      while (grp.ContainsSubExpressions()) {
        if (grp->GetDepth() == depth) { return true; }
        grp = grp->GetSubExpressionContaining(this);
      }
      return false;
    }

  private:
    Puzzle *puzzle;
    int index = 0;


    //-------------------------------------------------------------------------
    // CONVENIENCE PROPERTIES
    //-------------------------------------------------------------------------
	public:

    Op GetOpRight() {
		return opRight[(int)puzzle.Difficulty];
	}

	void SetOpRight(Op value) {
		opRight[(int)Difficulty.Easy] = value;
        opRight[(int)Difficulty.Medium] = value;
        opRight[(int)Difficulty.Hard] = value;
    }

    Op GetOpBottom { 
		return opBottom[(int)puzzle.Difficulty]; 
	}

	void SetOpBottom(Op value)
	{
        opBottom[(int)Difficulty.Easy] = value;
        opBottom[(int)Difficulty.Medium] = value;
        opBottom[(int)Difficulty.Hard] = value;
    }

  }

  class SideHelper 
  {
  public:
	static bool IsSource(Cube::Side side) {
      return side == Cube::Side::RIGHT || side == Cube::Side::BOTTOM;
    }
  private:
	  SideHelper();
  };

}



