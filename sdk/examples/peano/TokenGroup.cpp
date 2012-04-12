#include "TokenGroup.h"
#include "View.h"
#include "TokenView.h"

namespace TotalsGame
{
    extern Int2 kSideToUnit[4];
    
    
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
	
    bool TokenGroup::TokenAt(Int2 p, Token **t)
	{
      return
		  src->TokenAt(p - srcPos, t) ||
        dst->TokenAt(p - dstPos, t);
    }

    bool TokenGroup::PositionOf(Token *t, Int2 *p)
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

     TokenGroup::TokenGroup(
         IExpression *src, Int2 srcPos, Token *srcToken, unsigned srcSide,
         IExpression *dst, Int2 dstPos, Token *dstToken,
         Fraction val,
         ShapeMask mask
         )
     {
             this->src = src;
             this->srcPos = srcPos;
             this->srcToken = srcToken;
             this->srcSide = srcSide;
             this->dst = dst;
             this->dstPos = dstPos;
             this->dstToken = dstToken;
             void *userData = NULL;
             mValue = val;
             mMask = mask;
             mDepth = MAX(src->GetDepth(), dst->GetDepth()) +1;
     }

     void TokenGroup::RecomputeValue() {
         if (src->IsTokenGroup()) { ((TokenGroup*)src)->RecomputeValue(); }
         if (dst->IsTokenGroup()) { ((TokenGroup*)dst)->RecomputeValue(); }
         mValue = OpHelper::Compute(src->GetValue(), srcSide == RIGHT ? srcToken->GetOpRight() : srcToken->GetOpBottom(), dst->GetValue());
     }

     TokenGroup::TokenGroup(
         IExpression *src, Token *srcToken, unsigned srcSide,
         IExpression *dst, Token *dstToken
         )
     {
             this->src = src;
             this->srcToken = srcToken;
             this->srcSide = srcSide;
             this->dst = dst;
             this->dstToken = dstToken;
             void *userData = NULL;
             Int2 sp, dp;
             src->PositionOf(srcToken, &sp);
             dst->PositionOf(dstToken, &dp);
             Int2 d = kSideToUnit[srcSide];
             ShapeMask::TryConcat(src->GetMask(), dst->GetMask(), sp + d - dp, &mMask, &srcPos, &dstPos);
             mValue = OpHelper::Compute(src->GetValue(), srcSide == RIGHT ? srcToken->GetOpRight() : srcToken->GetOpBottom(), dst->GetValue());
             mDepth = MAX(src->GetDepth(), dst->GetDepth()) +1;
     }

     TokenGroup *TokenGroup::Connect(IExpression *src, Token *srcToken, Int2 d, IExpression *dst, Token *dstToken) {
         if (d.x < 0 || d.y < 0) { return Connect(dst, dstToken, d * -1, src, srcToken); }
         ShapeMask mask;
         Int2 d1, d2;
         Int2 sp, dp;
         src->PositionOf(srcToken, &sp);
         dst->PositionOf(dstToken, &dp);
         if (!ShapeMask::TryConcat(src->GetMask(), dst->GetMask(), sp + d - dp, &mask, &d1, &d2)) {
             return NULL;
         }
         unsigned srcSide = d.x == 1 ? RIGHT : BOTTOM;

         return new TokenGroup(
             src, d1, srcToken, srcSide,
             dst, d2, dstToken,
             OpHelper::Compute(src->GetValue(), srcSide == RIGHT ? srcToken->GetOpRight() : srcToken->GetOpBottom(), dst->GetValue()),
             mask
             );
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
