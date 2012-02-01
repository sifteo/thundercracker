#include "TokenGroup.h"

namespace TotalsGame
{
	DEFINE_POOL(TotalsGame)

	
    bool TokenGroup::TokenAt(const Vec2 &p, Token **t) 
	{
      return
		  src->TokenAt(p - srcPos, t) ||
        dst->TokenAt(p - dstPos, t);
    }

	bool TokenGroup::PositionOf(Token *t, Vec2 *p) 
	{
      if (src->PositionOf(t, p)) 
	  {
        p = p + srcPos;
        return true;
      } 
	  else if (dst->PositionOf(t, p)) 
	  {
        p = p + dstPos;
        return true;
      }
      return false;
    }

	bool TokenGroup::Contains(Token *t) 
	{
      return src->Contains(t) || dst->Contains(t);
    }

	 IExpression *GetSubExpressionContaining(Token *t) 
	{
      if (src->Contains(t)) 
	  {
		  return src; 
	  }
      if (dst->Contains(t)) 
	  {
		  return dst; 
	  }
      return NULL;
    }

	void TokenGroup::SetCurrent(IExpression *exp) 
	{
      src->SetCurrent(exp);
      dst->SetCurrent(exp);
    }
}