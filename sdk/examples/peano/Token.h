#include "IExpresson.h"

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
      var grp = current as TokenGroup;
      if (grp != null) { current = grp.GetSubExpressionContaining(this); }
    }

    public SideStatus StatusOfSide(Cube.Side side, IExpression current) {
      if (current == this) { return SideStatus.Open; }
      Int2 pos;
      current.PositionOf(this, out pos);
      Int2 del = Int2.Side(side);
      if (!current.Mask.BitAt(pos+del)) {
        return SideStatus.Open;
      }
      var grp = current as TokenGroup;
      if (side.IsSource()) {
        while(grp != null) {
          if (grp.srcToken == this && grp.srcSide == side) { return SideStatus.Connected; }
          grp = grp.GetSubExpressionContaining(this) as TokenGroup;
        }
      } else {
        side = CubeHelper.InvertSide(side);
        while(grp != null) {
          if (grp.dstToken == this && grp.srcSide == side) { return SideStatus.Connected; }
          grp = grp.GetSubExpressionContaining(this) as TokenGroup;
        }
      }
      return SideStatus.Blocked;
    }

    public bool ConnectsOnSideAtDepth(Cube.Side s, int depth, IExpression exp) {
      if (exp == null) {
        exp = current;
      }
      var grp = exp as TokenGroup;
      if (s.IsSource()) {
        while(grp != null) {
          if (grp.srcToken == this && grp.srcSide == s && depth >= grp.Depth && CheckDepth(depth, exp)) { return true; }
          grp = grp.GetSubExpressionContaining(this) as TokenGroup;
        }
      } else {
        s = CubeHelper.InvertSide(s);
        while(grp != null) {
          if (grp.dstToken == this && grp.srcSide == s && depth >= grp.Depth && CheckDepth(depth, exp)) { return true; }
          grp = grp.GetSubExpressionContaining(this) as TokenGroup;
        }
      }
      return false;
    }

    bool CheckDepth(int depth, IExpression exp) {
      var grp = exp as TokenGroup;
      while (grp != null) {
        if (grp.Depth == depth) { return true; }
        grp = grp.GetSubExpressionContaining(this) as TokenGroup;
      }
      return false;
    }

  private:
    Puzzle *puzzle;
    int index = 0;


    //-------------------------------------------------------------------------
    // CONVENIENCE PROPERTIES
    //-------------------------------------------------------------------------

    public Op OpRight {
      get { return opRight[(int)puzzle.Difficulty]; }
      set {
        opRight[(int)Difficulty.Easy] = value;
        opRight[(int)Difficulty.Medium] = value;
        opRight[(int)Difficulty.Hard] = value;
      }
    }
    public Op OpBottom {
      get { return opBottom[(int)puzzle.Difficulty]; }
      set {
        opBottom[(int)Difficulty.Easy] = value;
        opBottom[(int)Difficulty.Medium] = value;
        opBottom[(int)Difficulty.Hard] = value;
      }
    }

  }

  public static class SideHelper {
    public static bool IsSource(this Cube.Side side) {
      return side == Cube.Side.RIGHT || side == Cube.Side.BOTTOM;
    }
  }

}



