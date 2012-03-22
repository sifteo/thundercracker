#include "Token.h"
#include "TokenGroup.h"
#include "Puzzle.h"

namespace TotalsGame {

DEFINE_POOL(Token)

Puzzle *Token::GetPuzzle()
{
    return puzzle;
}

int Token::GetIndex()
{
    return index;
}

Token::Token(Puzzle *p, int i)
{
    // instantiated by puzzle's constructor
    puzzle = p;
    index = i;
    current = this;
    val = 0;
    view = NULL;
    for(int i = 0; i < 3; i++)
    {
        opRight[i] = OpAdd;
        opBottom[i] = OpSubtract;
    }
}

// IExpression impl
TotalsGame::Fraction Token::GetValue()
{
    return val;
}

int Token::GetDepth()
{
    return -1;
}

int Token::GetCount()
{
    return 1;
}

ShapeMask Token::GetMask()
{
    return ShapeMask::Unity;
}

bool Token::TokenAt(Int2 p, Token **t)
{
    if (p.x == 0 && p.y == 0)
    {
        *t = this;
        return true;
    }
    *t = NULL;
    return false;
}

bool Token::PositionOf(Token *t, Int2 *p)
{
    p->set(0,0);
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
    if (puzzle->target == NULL)
    {
        return current->GetValue();
    }
    return current == this ? puzzle->target->GetValue() : current->GetValue();
}

void Token::PopGroup() {
    if (current->IsTokenGroup())
    {
        current = ((TokenGroup*)current)->GetSubExpressionContaining(this);
    }
}

SideStatus Token::StatusOfSide(Cube::Side side, IExpression *current)
{
    if (current == this)
    {
        return SideStatusOpen;
    }
    Int2 pos;
    current->PositionOf(this, &pos);
    Int2 del = kSideToUnit[side];
    if (!current->GetMask().BitAt(pos+del))
    {
        return SideStatusOpen;
    }
    IExpression *grp = current;
    if (SideHelper::IsSource(side))
    {
        while(grp->IsTokenGroup())
        {
            TokenGroup *tokenGroup = (TokenGroup*)grp;
            if (tokenGroup->GetSrcToken() == this && tokenGroup->GetSrcSide() == side)
            {
                return SideStatusConnected;
            }
            grp = tokenGroup->GetSubExpressionContaining(this);
        }
    }
    else
    {
        side = (side + 2)&3;	//invert side
        while(grp->IsTokenGroup())
        {
            TokenGroup *tokenGroup = (TokenGroup*)grp;
            if (tokenGroup->GetDstToken() == this && tokenGroup->GetSrcSide() == side)
            {
                return SideStatusConnected;
            }
            grp = tokenGroup->GetSubExpressionContaining(this);
        }
    }
    return SideStatusBlocked;
}

bool Token::ConnectsOnSideAtDepth(Cube::Side s, int depth, IExpression *exp)
{
    if (exp == NULL)
    {
        exp = current;
    }
    IExpression *grp = exp;
    if (SideHelper::IsSource(s))
    {
        while(grp->IsTokenGroup())
        {
            TokenGroup *tokenGroup = (TokenGroup*)grp;
            if (tokenGroup->GetSrcToken() == this && tokenGroup->GetSrcSide() == s && depth >= tokenGroup->GetDepth() && CheckDepth(depth, exp))
            {
                return true;
            }
            grp = tokenGroup->GetSubExpressionContaining(this);
        }
    }
    else
    {
        s = (s+2)&3; //invert side
        while(grp->IsTokenGroup())
        {
            TokenGroup *tokenGroup = (TokenGroup*)grp;
            if (tokenGroup->GetDstToken() == this && tokenGroup->GetSrcSide() == s && depth >= tokenGroup->GetDepth() && CheckDepth(depth, exp))
            {
                return true;
            }
            grp = tokenGroup->GetSubExpressionContaining(this);
        }
    }
    return false;
}

bool Token::CheckDepth(int depth, IExpression *exp)
{
    IExpression *grp = exp;
    while (grp->IsTokenGroup())
    {
        if (grp->GetDepth() == depth)
        {
            return true;
        }
        grp = ((TokenGroup*)grp)->GetSubExpressionContaining(this);
    }
    return false;
}



//-------------------------------------------------------------------------
// CONVENIENCE PROPERTIES
//-------------------------------------------------------------------------

Op Token::GetOpRight()
{
    return opRight[(int)puzzle->GetDifficulty()];
}

void Token::SetOpRight(Op value)
{
    opRight[(int)DifficultyEasy] = value;
    opRight[(int)DifficultyMedium] = value;
    opRight[(int)DifficultyHard] = value;
}

void Token::SetOpRight(Difficulty dif, Op value)
{
    opRight[(int)dif] = value;
}

Op Token::GetOpBottom()
{
    return opBottom[(int)puzzle->GetDifficulty()];
}

void Token::SetOpBottom(Op value)
{
    opBottom[(int)DifficultyEasy] = value;
    opBottom[(int)DifficultyMedium] = value;
    opBottom[(int)DifficultyHard] = value;
}

void Token::SetOpBottom(Difficulty dif, Op value)
{
    opBottom[(int)dif] = value;
}


bool SideHelper::IsSource(Cube::Side side)
{
    return side == SIDE_RIGHT || side == SIDE_BOTTOM;
}


}
