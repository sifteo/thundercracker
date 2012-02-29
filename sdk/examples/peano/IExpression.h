#pragma once

#include "sifteo.h"
#include "ShapeMask.h"
#include "Fraction.h"

namespace TotalsGame {

	class Token;

	class IExpression {
	public:
        virtual Fraction GetValue() {return Fraction();}
        virtual int GetDepth() {return 0;}
        virtual int GetCount() {return 0;}
        virtual ShapeMask GetMask() {return ShapeMask();}
        virtual bool TokenAt(const Vec2 &p, Token **t) {return false;}
        virtual bool PositionOf(Token *t, Vec2 *p) {return false;}
        virtual bool Contains(Token *t) {return false;}
        virtual void SetCurrent(IExpression *exp) {}

		//ugh
		virtual bool IsTokenGroup() {return false;}		
	};

}
