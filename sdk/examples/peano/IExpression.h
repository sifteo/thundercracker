#include "sifteo.h"
#include "ShapeMask.h"
#include "Fraction.h"

namespace TotalsGame {

	class Token;

	class IExpression {
	public:
		virtual Fraction GetValue() = 0;
		virtual int GetDepth() = 0;
		virtual int GetCount() = 0;
		virtual ShapeMask GetMask() = 0;
		virtual bool TokenAt(const Vec2 &p, Token **t) = 0;
		virtual bool PositionOf(Token *t, Vec2 *p) = 0;
		virtual bool Contains(Token *t) = 0;
		virtual void SetCurrent(IExpression *exp) = 0;

		virtual bool ContainsSubExpressions() {return false;}
		virtual IExpression *GetSubExpressionContaining(Token *token) {return NULL;}
	};

	struct Connection {
		Vec2 pos;
		Vec2 dir;

		bool Matches(const Connection &c) {
			//return (dir - c.dir).Norm() == 0;
			return dir.x == c.dir.x && dir.y == c.dir.y;
		}

		bool IsFromOrigin() {
			return pos.x == 0 && pos.y == 0;
		}

		bool IsBottom() {
			return dir.x == 0 && dir.y == 1;
		}

		bool IsRight() {
			return dir.x == 1 && dir.y == 0;
		}
	};

}
