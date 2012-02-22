#pragma once

#include "Fraction.h"
#include "IExpression.h"
#include "Token.h"
#include "ObjectPool.h"

namespace TotalsGame 
{

	class TokenGroup : public IExpression 
	{
        DECLARE_POOL(TokenGroup, Game::NUMBER_OF_CUBES)

	public:

		IExpression *GetSrc() {return src;}
		IExpression *GetDst() {return dst;}
		Vec2 GetSrcPos() {return srcPos;}
		Vec2 GetDstPos() {return dstPos;}
		Token *GetSrcToken() {return srcToken;}
		Cube::Side GetSrcSide() {return srcSide;}
		Token *GetDstToken() {return dstToken;}

		void *userData;

        //IExpression
		virtual Fraction GetValue();
		virtual ShapeMask GetMask();
		virtual int GetDepth();
		virtual int GetCount();
		virtual bool TokenAt(const Vec2 &p, Token **t);
		virtual bool PositionOf(Token *t, Vec2 *p);
		virtual bool Contains(Token *t);
        virtual void SetCurrent(IExpression *exp);
		virtual bool IsTokenGroup() {return true;}
        
		IExpression *GetSubExpressionContaining(Token *t);

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
        void AlertWillJoinGroup();
        void AlertDidJoinGroup();
        void AlertDidGroupDisconnect();

		static TokenGroup *Connect(Token *st, Vec2 d, Token *dt) { return Connect(st, st, d, dt, dt); }
		static TokenGroup *Connect(IExpression *src, Token *st, Vec2 d, Token *dt) { return Connect(src, st, d, dt, dt); }

        static TokenGroup *Connect(IExpression *src, Token *srcToken, Vec2 d, IExpression *dst, Token *dstToken);

        void RecomputeValue();

		TokenGroup(
			IExpression *src, Token *srcToken, Cube::Side srcSide,
			IExpression *dst, Token *dstToken
            );

		TokenGroup(
			IExpression *src, Vec2 srcPos, Token *srcToken, Cube::Side srcSide,
			IExpression *dst, Vec2 dstPos, Token *dstToken,
			Fraction val,
			ShapeMask mask
            );

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
				Fraction ret;
				switch(op)
				{
				case OpAdd:
					ret = left + right;
					break;
				case OpSubtract:
					ret = left - right;
					break;
				case OpMultiply:
					ret = left * right;
					break;
				case OpDivide:
					ret = left / right;
					break;
				}				
				return ret;
			}
		};

	};

}
