#pragma once

#include "Fraction.h"
#include "IExpression.h"
#include "Token.h"
#include "ObjectPool.h"

namespace TotalsGame 
{

	class TokenGroup : public IExpression 
	{
        DECLARE_POOL(TokenGroup, 2 * NUM_CUBES)

	public:

		IExpression *GetSrc() {return src;}
		IExpression *GetDst() {return dst;}
        Vector2<int> GetSrcPos() {return srcPos;}
        Vector2<int> GetDstPos() {return dstPos;}
		Token *GetSrcToken() {return srcToken;}
		Cube::Side GetSrcSide() {return srcSide;}
		Token *GetDstToken() {return dstToken;}


        //IExpression
		virtual Fraction GetValue();
		virtual ShapeMask GetMask();
		virtual int GetDepth();
		virtual int GetCount();
        virtual bool TokenAt(Vector2<int> p, Token **t);
        virtual bool PositionOf(Token *t, Vector2<int> *p);
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

        static TokenGroup *Connect(Token *st, Vector2<int> d, Token *dt) { return Connect(st, st, d, dt, dt); }
        static TokenGroup *Connect(IExpression *src, Token *st, Vector2<int> d, Token *dt) { return Connect(src, st, d, dt, dt); }

        static TokenGroup *Connect(IExpression *src, Token *srcToken, Vector2<int> d, IExpression *dst, Token *dstToken);

        void RecomputeValue();

		TokenGroup(
			IExpression *src, Token *srcToken, Cube::Side srcSide,
			IExpression *dst, Token *dstToken
            );

		TokenGroup(
            IExpression *src, Vector2<int> srcPos, Token *srcToken, Cube::Side srcSide,
            IExpression *dst, Vector2<int> dstPos, Token *dstToken,
			Fraction val,
			ShapeMask mask
            );

		virtual ~TokenGroup() {}

	private:
		IExpression *src;
		IExpression *dst;
        Vector2<int> srcPos;
        Vector2<int> dstPos;
		Token *srcToken;
		Cube::Side srcSide;
		Token *dstToken;

		Fraction mValue; // mutable because it changes when the difficulty changes

		ShapeMask mMask;
		int mDepth;


		class OpHelper {
		public:
            static Fraction Compute(Fraction left, Op op, Fraction right);
		};

	};

}
