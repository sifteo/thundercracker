#pragma once

#include "sifteo.h"
#include "ShapeMask.h"
#include "Fraction.h"

namespace TotalsGame {

	class Token;

	class IExpression {
	public:
		virtual Fraction GetValue();// = 0;
		virtual int GetDepth();// = 0;
		virtual int GetCount();// = 0;
		virtual ShapeMask GetMask();// = 0;
		virtual bool TokenAt(const Vec2 &p, Token **t);// = 0;
		virtual bool PositionOf(Token *t, Vec2 *p);// = 0;
		virtual bool Contains(Token *t);// = 0;
		virtual void SetCurrent(IExpression *exp);// = 0;

		//ugh
		virtual bool IsTokenGroup() {return false;}		
	};

}
