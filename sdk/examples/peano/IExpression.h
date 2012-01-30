#include "sifteo.h"

namespace TotalsGame {
  
  class IExpression {
    virtual Fraction GetValue() = 0;
    virtual int GetDepth() = 0;
    virtual int GetCount() = 0;
    virtual ShapeMask GetMask() = 0;
    virtual bool TokenAt(const Int2 &p, Token **t) = 0;
    virtual bool PositionOf(Token *t, Int2 *p) = 0;
    virtual bool Contains(Token *t) = 0;
    virtual void SetCurrent(IExpression *exp) = 0;

	virtual bool ContainsSubExpressions() {return false;}
	virtual IExpression *GetSubExpressionContaining(Token *token) {return null;}
  }
  
  struct Connection {
    Vec2 pos;
    Vec2 dir;
    
    bool Matches(const Connection &c) {
      return (dir - c.dir).Norm() == 0;
    }

    bool IsFromOrigin() {
      return pos.x == 0 && pos.y == 0;
    }

    bool IsBottom() {
      return dir.x == 0 && dir.y == 1;
    }

    bool IsRight() {
      return dir.x == 1 && dir.y == 0;
    }
  }
  
}
