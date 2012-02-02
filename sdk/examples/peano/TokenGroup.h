#pragma once

#include "Fraction.h"
#include "IExpression.h"
#include "Token.h"
#include "ObjectPool.h"

namespace TotalsGame 
{

	class TokenGroup : public IExpression 
	{
		DECLARE_POOL(TokenGroup, 32)

	public:

		IExpression *GetSrc() {return src;}
		IExpression *GetDst() {return dst;}
		Vec2 GetSrcPos() {return srcPos;}
		Vec2 GetDstPos() {return dstPos;}
		Token *GetSrcToken() {return srcToken;}
		Cube::Side GetSrcSide() {return srcSide;}
		Token *GetDstToken() {return dstToken;}

		void *userData;

		Fraction GetValue() { return mValue; }
		ShapeMask GetMask() { return mMask; }
		int GetDepth() { return mDepth;}
		int GetCount() { return src->GetCount() + dst->GetCount();}

		bool TokenAt(const Vec2 &p, Token **t);

		bool PositionOf(Token *t, Vec2 *p);

		bool Contains(Token *t);

		IExpression *GetSubExpressionContaining(Token *t);
		bool IsTokenGroup() {return true;}
		/*
		public IEnumerable<Token> Tokens {
		get {
		var token = src as Token;
		if (token == null) {
		foreach(var t in (src as TokenGroup).Tokens) { yield return t; }
		} else { yield return token; }
		token = dst as Token;
		if (token == null) {
		foreach(var t in (dst as TokenGroup).Tokens) { yield return t; }
		} else { yield return token; }
		}
		}
		*/
		void SetCurrent(IExpression *exp);

		static TokenGroup *Connect(Token *st, Vec2 d, Token *dt) { return Connect(st, st, d, dt, dt); }
		static TokenGroup *Connect(IExpression *src, Token *st, Vec2 d, Token *dt) { return Connect(src, st, d, dt, dt); }

		static TokenGroup *Connect(IExpression *src, Token *srcToken, Vec2 d, IExpression *dst, Token *dstToken) {
			if (d.x < 0 || d.y < 0) { return Connect(dst, dstToken, -d, src, srcToken); }
			ShapeMask mask;
			Vec2 d1, d2;
			Vec2 sp, dp;
			src->PositionOf(srcToken, &sp);
			dst->PositionOf(dstToken, &dp);
			if (!ShapeMask::TryConcat(src->GetMask(), dst->GetMask(), sp + d - dp, &mask, &d1, &d2)) {
				return NULL;
			}
			Cube::Side srcSide = d.x == 1 ? SIDE_RIGHT : SIDE_BOTTOM;
			return new TokenGroup(
				src, d1, srcToken, srcSide,
				dst, d2, dstToken,
				OpHelper::Compute(src->GetValue(), srcSide == SIDE_RIGHT ? srcToken->GetOpRight() : srcToken->GetOpBottom(), dst->GetValue()),
				mask
				);
		}

		void RecomputeValue() {
			if (src->IsTokenGroup()) { ((TokenGroup*)src)->RecomputeValue(); }
			if (dst->IsTokenGroup()) { ((TokenGroup*)dst)->RecomputeValue(); }
			mValue = OpHelper::Compute(src->GetValue(), srcSide == SIDE_RIGHT ? srcToken->GetOpRight() : srcToken->GetOpBottom(), dst->GetValue());
		}

		TokenGroup(
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
				Vec2 sp, dp;
				src->PositionOf(srcToken, &sp);
				dst->PositionOf(dstToken, &dp);
				Vec2 d = kSideToUnit[srcSide];
				ShapeMask::TryConcat(src->GetMask(), dst->GetMask(), sp + d - dp, &mMask, &srcPos, &dstPos);
				mValue = OpHelper::Compute(src->GetValue(), srcSide == SIDE_RIGHT ? srcToken->GetOpRight() : srcToken->GetOpBottom(), dst->GetValue());				
				mDepth = max(src->GetDepth(), dst->GetDepth()) +1;
		}

		TokenGroup(
			IExpression *src, Vec2 srcPos, Token *srcToken, Cube::Side srcSide,
			IExpression *dst, Vec2 dstPos, Token *dstToken,
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
				mDepth = max(src->GetDepth(), dst->GetDepth()) +1;
		}

		virtual ~TokenGroup() {}

	private:
		IExpression *src;
		IExpression *dst;
		Vec2 srcPos;
		Vec2 dstPos;
		Token *srcToken;
		Cube::Side srcSide;
		Token *dstToken;

		Fraction mValue; // mutable because it changes when the difficulty changes

		ShapeMask mMask;
		int mDepth;


		class OpHelper {
		public:
			static Fraction Compute(Fraction left, Op op, Fraction right) 
			{
				switch(op)
				{
				case OpAdd:
					return left + right;
				case OpSubtract:
					return left - right;
				case OpMultiply:
					return left * right;
				case OpDivide:
					return left / right;
				}
			}
		};

	};

}
