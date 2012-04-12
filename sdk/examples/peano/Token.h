#pragma once

#include "sifteo.h"
#include "IExpression.h"
#include "ObjectPool.h"
#include "Game.h"

namespace TotalsGame {

	class Puzzle;
    class View;
    class TokenView;

	enum SideStatus {
		SideStatusOpen,       // physically no tokens are on this side
		SideStatusConnected,  // this side is participating in the solution
		SideStatusBlocked     // this side is NOT participating in the solution, but it physically blocked
	};

	enum Op {
		OpAdd = 0,
		OpSubtract = 1,
		OpMultiply = 2,
		OpDivide = 3
	};

	class Token : public IExpression {

        DECLARE_POOL(Token, NUM_CUBES);

	public:

		Puzzle *GetPuzzle();
		int GetIndex();

		Op opRight[3];
		Op opBottom[3];
		int val;
		IExpression *current;

        View *view;
        TokenView *GetTokenView() {return (TokenView*)view;}

		//ONLY FOR PUZZLE.H USE
		Token(Puzzle *p, int i);
		Token();
		virtual ~Token() {}

		// IExpression impl

		virtual Fraction GetValue();
		virtual int GetDepth();
		virtual int GetCount();
		virtual ShapeMask GetMask();
        virtual bool TokenAt(Int2 p, Token **t);
        virtual bool PositionOf(Token *t, Int2 *p);
		virtual bool Contains(Token *t);
		virtual void SetCurrent(IExpression *exp);

		// getters
		Fraction GetCurrentTotal();

		void PopGroup();

		SideStatus StatusOfSide(Cube::Side side, IExpression *current);
		bool ConnectsOnSideAtDepth(Cube::Side s, int depth, IExpression *exp);

		bool CheckDepth(int depth, IExpression *exp);

    private:
		Puzzle *puzzle;
		int index;

		//-------------------------------------------------------------------------
		// CONVENIENCE PROPERTIES
		//-------------------------------------------------------------------------
	public:

		Op GetOpRight();
		void SetOpRight(Op value);
        void SetOpRight(Difficulty dif, Op value);
		Op GetOpBottom();
		void SetOpBottom(Op value);
        void SetOpBottom(Difficulty dif, Op value);
	};

	class SideHelper 
	{
	public:
		static bool IsSource(Cube::Side side);
	private:
		SideHelper();	//non-instantiable
	};

}



