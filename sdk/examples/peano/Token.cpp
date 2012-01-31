#include "Puzzle.h"
#include "Token.h"

void * operator new (size_t, void * p) throw() 
{
	return p;
}

namespace TotalsGame {

	Puzzle *Token::GetPuzzle() 
	{
		return puzzle;
	}

	int Token::GetIndex() 
	{
		return indexer;
	}

	Token::Token(Puzzle *p, int i) 
	{
		// instantiated by puzzle's constructor
		puzzle = p;
		index = i;
		current = this;
		val = 0;
		userData = NULL;
		for(int i = 0; i < 3; i++)
		{
			opRight[i] = Op::Add;
			opBottom[i] = Op::Subtract
		}
	}

	// IExpression impl
	Fraction Token::GetValue 
	{
		return val;
	}

	int Token::GetDepth 
	{
		return -1;
	}

	int Token::GetCount 
	{
		return 1;
	}

	ShapeMask Token::GetMask 
	{
		return ShapeMask.Unity;
	}

	bool Token::TokenAt(const Int2 &p, Token **t) 
	{
		if (p.x == 0 && p.y == 0) 
		{
			*t = this;
			return true;
		}
		*t = null;
		return false;
	}

	bool Token::PositionOf(Token *t, Int2 *p) 
	{
		*p = Vec2.Zero;
		return t == this;
	}

	bool Token::Contains(Token *t) 
	{
		return t == this;
	}

	void Token::SetCurrent(IExpression *exp) 
	{
		current = exp;
	}

	// getters

	Fraction Token::GetCurrentTotal() 
	{
		if (puzzle->target == null) 
		{
			return current->GetValue();
		}
		return current == this ? puzzle->target->GetValue() : current->GetValue();
	}

	void Token::PopGroup() {
		if (current->ContainsSubExpressions())
		{
			current = current->GetSubExpressionContaining(this);
		}
	}

	SideStatus Token::StatusOfSide(Cube::Side side, IExpression *current) 
	{
		if (current == this)
		{ 
			return SideStatus.Open;
		}
		Vec2 pos;
		current->PositionOf(this, &pos);
		Vec2 del = Vec2.Side(side);
		if (!current->Mask.BitAt(pos+del))
		{
			return SideStatus::Open;
		}
		IExpression *grp = current;
		if (side->IsSource()) 
		{
			while(grp->ContainsSubExpressions())
			{
				if (grp->srcToken == this && grp->srcSide == side) 
				{ 
					return SideStatus::Connected; 
				}
				grp = grp->GetSubExpressionContaining(this);
			}
		} 
		else
		{
			side = CubeHelper::InvertSide(side);
			while(grp->ContainsSubExpressions()) 
			{
				if (grp->dstToken == this && grp->srcSide == side)
				{
					return SideStatus::Connected;
				}
				grp = grp->GetSubExpressionContaining(this);
			}
		}
		return SideStatus::Blocked;
	}

	bool Token::ConnectsOnSideAtDepth(Cube::Side s, int depth, IExpression *exp) 
	{
		if (exp == null)
		{
			exp = current;
		}
		IExpression *grp = exp;
		if (s->IsSource())
		{
			while(grp.ContainsSubExpressions())
			{
				if (grp->srcToken == this && grp->srcSide == s && depth >= grp->GetDepth() && CheckDepth(depth, exp))
				{ 
					return true;
				}
				grp = grp->GetSubExpressionContaining(this);
			}
		} 
		else
		{
			s = CubeHelper::InvertSide(s);
			while(grp->ContainsSubExpressions())
			{
				if (grp->dstToken == this && grp->srcSide == s && depth >= grp->GetDepth() && CheckDepth(depth, exp))
				{
					return true; 
				}
				grp = grp->GetSubExpressionContaining(this);
			}
		}
		return false;
	}

	bool Token::CheckDepth(int depth, IExpression *exp)
	{
		IExpression *grp = exp;
		while (grp.ContainsSubExpressions())
		{
			if (grp->GetDepth() == depth)
			{ 
				return true; 
			}
			grp = grp->GetSubExpressionContaining(this);
		}
		return false;
	}



	//-------------------------------------------------------------------------
	// CONVENIENCE PROPERTIES
	//-------------------------------------------------------------------------

	Op Token::GetOpRight()
	{
		return opRight[(int)puzzle.Difficulty];
	}

	void Token::SetOpRight(Op value)
	{
		opRight[(int)Difficulty.Easy] = value;
		opRight[(int)Difficulty.Medium] = value;
		opRight[(int)Difficulty.Hard] = value;
	}

	Op Token::GetOpBottom 
	{ 
		return opBottom[(int)puzzle.Difficulty]; 
	}

	void Token::SetOpBottom(Op value)
	{
		opBottom[(int)Difficulty.Easy] = value;
		opBottom[(int)Difficulty.Medium] = value;
		opBottom[(int)Difficulty.Hard] = value;
	}


	static bool SideHelper::IsSource(Cube::Side side)
	{
		return side == Cube::Side::RIGHT || side == Cube::Side::BOTTOM;
	}
};

}



