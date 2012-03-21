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
	
    bool TokenGroup::TokenAt(Vector2<int> p, Token **t)
	{
      return
		  src->TokenAt(p - srcPos, t) ||
        dst->TokenAt(p - dstPos, t);
    }

    bool TokenGroup::PositionOf(Token *t, Vector2<int> *p)
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
         IExpression *src, Vector2<int> srcPos, Token *srcToken, Cube::Side srcSide,
         IExpression *dst, Vector2<int> dstPos, Token *dstToken,
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
         mValue = OpHelper::Compute(src->GetValue(), srcSide == SIDE_RIGHT ? srcToken->GetOpRight() : srcToken->GetOpBottom(), dst->GetValue());
     }

     TokenGroup::TokenGroup(
         IExpression *src, Token *srcToken, Cube::Side srcSide,
         IExpression *dst, Token *dstToken
         )
     {
             this->src = src;
             this->srcToken = srcToken;
             this->srcSide = srcSide;
             this->dst = dst;
             this->dstToken = dstToken;
             void *userData = NULL;
             Vector2<int> sp, dp;
             src->PositionOf(srcToken, &sp);
             dst->PositionOf(dstToken, &dp);
             Vector2<int> d = kSideToUnit[srcSide];
#if NO_STACK_PARAMS_HACK
             ShapeMask::m1 = src->GetMask();
             ShapeMask::m2 = dst->GetMask();
             ShapeMask::TryConcat(sp + d - dp, &mMask, &srcPos, &dstPos);
#else
             ShapeMask::TryConcat(src->GetMask(), dst->GetMask(), sp + d - dp, &mMask, &srcPos, &dstPos);
#endif
             mValue = OpHelper::Compute(src->GetValue(), srcSide == SIDE_RIGHT ? srcToken->GetOpRight() : srcToken->GetOpBottom(), dst->GetValue());
             mDepth = MAX(src->GetDepth(), dst->GetDepth()) +1;
     }

     TokenGroup *TokenGroup::Connect(IExpression *src, Token *srcToken, Vector2<int> d, IExpression *dst, Token *dstToken) {
         if (d.x < 0 || d.y < 0) { return Connect(dst, dstToken, d * -1, src, srcToken); }
         ShapeMask mask;
         Vector2<int> d1, d2;
         Vector2<int> sp, dp;
         src->PositionOf(srcToken, &sp);
         dst->PositionOf(dstToken, &dp);
#if NO_STACK_PARAMS_HACK
         ShapeMask::m1 = src->GetMask();
         ShapeMask::m2 = dst->GetMask();
         if (!ShapeMask::TryConcat(sp + d - dp, &mask, &d1, &d2)) {
             return NULL;
         }
#else
         if (!ShapeMask::TryConcat(src->GetMask(), dst->GetMask(), sp + d - dp, &mask, &d1, &d2)) {
             return NULL;
         }
#endif
         Cube::Side srcSide = d.x == 1 ? SIDE_RIGHT : SIDE_BOTTOM;

         return new TokenGroup(
             src, d1, srcToken, srcSide,
             dst, d2, dstToken,
             OpHelper::Compute(src->GetValue(), srcSide == SIDE_RIGHT ? srcToken->GetOpRight() : srcToken->GetOpBottom(), dst->GetValue()),
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
