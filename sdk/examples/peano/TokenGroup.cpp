#include "TokenGroup.h"
#include "View.h"
#include "TokenView.h"

namespace TotalsGame
{
	DEFINE_POOL(TokenGroup)

    Fraction TokenGroup::GetValue()
    {
        return mValue;
    }
    
    ShapeMask TokenGroup::GetMask()
    {
        return mMask;
    }
    
    int TokenGroup::GetDepth()
    {
        return mDepth;
    }
    
    int TokenGroup::GetCount()
    {
        return src->GetCount() + dst->GetCount();
    }
	
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
        *p = *p + srcPos;
        return true;
      } 
	  else if (dst->PositionOf(t, p)) 
	  {
        *p = *p + dstPos;
        return true;
      }
      return false;
    }

	bool TokenGroup::Contains(Token *t) 
	{
      return src->Contains(t) || dst->Contains(t);
    }

	 IExpression *TokenGroup::GetSubExpressionContaining(Token *t) 
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

    void TokenGroup::AlertWillJoinGroup()
    {
        if(src->IsTokenGroup())
        {
            ((TokenGroup*)src)->AlertWillJoinGroup();
        }
        else
        {
            ((TokenView*)((Token*)src)->view)->WillJoinGroup();
        }

        if(dst->IsTokenGroup())
        {
            ((TokenGroup*)dst)->AlertWillJoinGroup();
        }
        else
        {
            ((TokenView*)((Token*)dst)->view)->WillJoinGroup();
        }
    }

    void TokenGroup::AlertDidJoinGroup()
    {
        if(src->IsTokenGroup())
        {
            ((TokenGroup*)src)->AlertDidJoinGroup();
        }
        else
        {
            ((TokenView*)((Token*)src)->view)->DidJoinGroup();
        }

        if(dst->IsTokenGroup())
        {
            ((TokenGroup*)dst)->AlertDidJoinGroup();
        }
        else
        {
            ((TokenView*)((Token*)dst)->view)->DidJoinGroup();
        }
    }

    void TokenGroup::AlertDidGroupDisconnect()
    {
        if(src->IsTokenGroup())
        {
            ((TokenGroup*)src)->AlertDidGroupDisconnect();
        }
        else
        {
            ((TokenView*)((Token*)src)->view)->DidGroupDisconnect();
        }

        if(dst->IsTokenGroup())
        {
            ((TokenGroup*)dst)->AlertDidGroupDisconnect();
        }
        else
        {
            ((TokenView*)((Token*)dst)->view)->DidGroupDisconnect();
        }
    }

}
