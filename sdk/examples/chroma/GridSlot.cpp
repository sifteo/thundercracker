/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "GridSlot.h"
#include "game.h"
#include "assets.gen.h"
#include "utils.h"
#include <stdlib.h>

const AssetImage *GridSlot::TEXTURES[ GridSlot::NUM_COLORS ] = 
{
	&Gem0,
	&Gem1,
	&Gem2,
	&Gem3,
	&Gem4,
	&Gem5,
	&Gem6,
	&Gem7,
};

const AssetImage *GridSlot::ROLLING_TEXTURES[ GridSlot::NUM_ROLLING_COLORS ] = 
{
	&RollingGem0,
	&RollingGem1,
};

GridSlot::GridSlot() : 
	m_state( STATE_GONE ),
	m_eventTime( 0.0f ),
	m_score( 0 ),
	m_bFixed( false )
{
	//TEST randomly make some empty ones
	/*if( Game::Rand(100) > 50 )
		m_state = STATE_LIVING;
	else
		m_state = STATE_GONE;*/
	m_color = Game::Rand(NUM_COLORS);
}


void GridSlot::Init( CubeWrapper *pWrapper, unsigned int row, unsigned int col )
{
	m_pWrapper = pWrapper;
	m_row = row;
	m_col = col;
	m_state = STATE_GONE;
}


void GridSlot::FillColor(unsigned int color)
{
	m_state = STATE_LIVING;
	m_color = color;
	m_bFixed = false;
}


const AssetImage &GridSlot::GetTexture() const
{
	return *TEXTURES[ m_color ];
}

//draw self on given vid at given vec
void GridSlot::Draw( VidMode_BG0 &vid, unsigned int tiltMask )
{
	Vec2 vec( m_col * 4, m_row * 4 );
	switch( m_state )
	{
		case STATE_LIVING:
		{
			const AssetImage &tex = GetTexture();
			if( IsFixed() )
				vid.BG0_drawAsset(vec, tex, 2);
			else
			{
				//only have some of these now
				if( m_color < NUM_ROLLING_COLORS )
				{
					const AssetImage &rolltex = *ROLLING_TEXTURES[ m_color ];
					unsigned int frame = CalculateRollFrame( tiltMask );
					vid.BG0_drawAsset(vec, rolltex, frame);

					//PRINT( "tilt bit mask %d, frame=%d\n", tiltMask, frame );
				}
				else
					vid.BG0_drawAsset(vec, tex, 0);
			}
			break;
		}
		case STATE_MOVING:
		{
			const AssetImage &tex = GetTexture();

			Vec2 curPos = Vec2( m_curMovePos.x, m_curMovePos.y );

			//PRINT( "drawing dot x=%d, y=%d\n", m_curMovePos.x, m_curMovePos.y );

			vid.BG0_drawAsset(curPos, tex, 0);
			break;
		}
		case STATE_MARKED:
		{
			const AssetImage &tex = GetTexture();
			if( IsFixed() )
				vid.BG0_drawAsset(vec, tex, 3);
			else
				vid.BG0_drawAsset(vec, tex, 1);
			break;
		}
		case STATE_EXPLODING:
		{
			const AssetImage &tex = GetTexture();
			vid.BG0_drawAsset(vec, tex, 4);
			break;
		}
		case STATE_SHOWINGSCORE:
		{
			char aStr[2];
			sprintf( aStr, "%d", m_score );
			vid.BG0_drawAsset(vec, GemEmpty, 0);
			vid.BG0_text(Vec2( vec.x + 1, vec.y + 1 ), Font, aStr);
			break;
		}
		/*case STATE_GONE:
		{
			vid.BG0_drawAsset(vec, GemEmpty, 0);
			break;
		}*/
		default:
			break;
	}
	
}


void GridSlot::Update(float t)
{
	switch( m_state )
	{
		case STATE_MOVING:
		{
			Vec2 vDiff = Vec2( m_col * 4 - m_curMovePos.x, m_row * 4 - m_curMovePos.y );

			if( vDiff.x != 0 )
				m_curMovePos.x += ( vDiff.x / abs( vDiff.x ) );
			else if( vDiff.y != 0 )
				m_curMovePos.y += ( vDiff.y / abs( vDiff.y ) );
			else
				m_state = STATE_LIVING;

			break;
		}
		case STATE_MARKED:
		{
			if( t - m_eventTime > MARK_SPREAD_DELAY )
                spread_mark();
            if( t - m_eventTime > MARK_BREAK_DELAY )
                explode();
			break;
		}
		case STATE_EXPLODING:
		{
			spread_mark();
			if( t - m_eventTime > MARK_EXPLODE_DELAY )
                die();
			break;
		}
		case STATE_SHOWINGSCORE:
		{
			if( t - m_eventTime > SCORE_FADE_DELAY )
			{
                m_state = STATE_GONE;
				m_pWrapper->checkEmpty();
			}
			break;
		}
		default:
			break;
	}
}


void GridSlot::mark()
{
	m_state = STATE_MARKED;
	m_eventTime = System::clock();
}


void GridSlot::spread_mark()
{
	if( m_state == STATE_MARKED || m_state == STATE_EXPLODING )
	{
		markNeighbor( m_row - 1, m_col );
		markNeighbor( m_row, m_col - 1 );
		markNeighbor( m_row + 1, m_col );
		markNeighbor( m_row, m_col + 1 );
	}
}

void GridSlot::explode()
{
	m_state = STATE_EXPLODING;
	m_eventTime = System::clock();
}

void GridSlot::die()
{
	m_state = STATE_SHOWINGSCORE;
	m_score = Game::Inst().getIncrementScore();
	Game::Inst().CheckChain( m_pWrapper );
	m_eventTime = System::clock();
}


void GridSlot::markNeighbor( int row, int col )
{
	//find my neighbor and see if we match
	GridSlot *pNeighbor = m_pWrapper->GetSlot( row, col );

	//PRINT( "pneighbor = %p", pNeighbor );
	//PRINT( "color = %d", pNeighbor->getColor() );
	if( pNeighbor && pNeighbor->isAlive() && pNeighbor->getColor() == m_color )
		pNeighbor->mark();
}



//copy color and some other attributes from target.  Used when tilting
void GridSlot::TiltFrom(GridSlot &src)
{
	m_state = STATE_PENDINGMOVE;
	m_color = src.m_color;
	m_eventTime = src.m_eventTime;
	m_curMovePos.x = src.m_col * 4;
	m_curMovePos.y = src.m_row * 4;
}


//if we have a move pending, start it
void GridSlot::startPendingMove()
{
	if( m_state == STATE_PENDINGMOVE )
	{
		m_state = STATE_MOVING;
	}
}


//given a tiltmask, calculate the roll frame we should be in
unsigned int GridSlot::CalculateRollFrame( unsigned int tiltMask ) const
{
	//put these in the order of the frames
	enum
	{
		FRAME_UPRIGHT,
		FRAME_RIGHT,
		FRAME_DOWNRIGHT,
		FRAME_DOWN,
		FRAME_DOWNLEFT,
		FRAME_LEFT,
		FRAME_UPLEFT,
		FRAME_UP,
		FRAME_NONE
	};

	//check diagonals

	//actually want to return the reverse the direction
	if( tiltMask & ( 1 << UP ) && tiltMask & ( 1 << LEFT ) )
		return FRAME_DOWNRIGHT;
	else if( tiltMask & ( 1 << UP ) && tiltMask & ( 1 << RIGHT ) )
		return FRAME_DOWNLEFT;
	else if( tiltMask & ( 1 << DOWN ) && tiltMask & ( 1 << LEFT ) )
		return FRAME_UPRIGHT;
	else if( tiltMask & ( 1 << DOWN ) && tiltMask & ( 1 << RIGHT ) )
		return FRAME_UPLEFT;
	else if( tiltMask & ( 1 << UP ) )
		return FRAME_DOWN;
	else if( tiltMask & ( 1 << RIGHT ) )
		return FRAME_LEFT;
	else if( tiltMask & ( 1 << LEFT ) )
		return FRAME_RIGHT;
	else if( tiltMask & ( 1 << DOWN ) )
		return FRAME_UP;

	return FRAME_NONE;
}