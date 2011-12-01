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


const float GridSlot::MARK_SPREAD_DELAY = 0.333333f;
const float GridSlot::MARK_BREAK_DELAY = 0.666666f;
const float GridSlot::MARK_EXPLODE_DELAY = 0.16666666f;
const float GridSlot::SCORE_FADE_DELAY = 2.0f;
const float GridSlot::EXPLODE_FRAME_LEN = ( GridSlot::MARK_BREAK_DELAY - GridSlot::MARK_SPREAD_DELAY ) / (float) GridSlot::NUM_EXPLODE_FRAMES;
const unsigned int GridSlot::NUM_ROLL_FRAMES = 16 * GridSlot::NUM_FRAMES_PER_ROLL_ANIM_FRAME;
//const unsigned int GridSlot::NUM_IDLE_FRAMES = 4 * GridSlot::NUM_FRAMES_PER_IDLE_ANIM_FRAME;
const float GridSlot::START_FADING_TIME = 1.75f;
const float GridSlot::FADE_FRAME_TIME = ( GridSlot::SCORE_FADE_DELAY - GridSlot::START_FADING_TIME ) / GridSlot::NUM_POINTS_FRAMES;




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


const AssetImage *GridSlot::ANIMATEDTEXTURES[ GridSlot::NUM_ANIMATED_COLORS ] =
{
    &Gem0,
};

const AssetImage *GridSlot::EXPLODINGTEXTURES[ GridSlot::NUM_EXPLODING_COLORS ] =
{
    &ExplodeGem0,
};


GridSlot::GridSlot() : 
	m_state( STATE_GONE ),
	m_eventTime( 0.0f ),
	m_score( 0 ),
	m_bFixed( false ),
	m_animFrame( 0 )
{
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


const AssetImage &GridSlot::GetExplodingTexture() const
{
    return *EXPLODINGTEXTURES[ m_color ];
}


//draw self on given vid at given vec
void GridSlot::Draw( VidMode_BG0 &vid, Float2 &tiltState )
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
                if( m_color < NUM_ANIMATED_COLORS )
				{
                    const AssetImage &animtex = *ANIMATEDTEXTURES[ m_color ];
                    unsigned int frame;
                    /*if( m_pWrapper->IsIdle() )
                        frame = GetIdleFrame();
                    else*/
                        frame = GetTiltFrame( tiltState );
                    vid.BG0_drawAsset(vec, animtex, frame);
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
            if( m_color < NUM_ANIMATED_COLORS )
			{
                const AssetImage &tex = *ANIMATEDTEXTURES[m_color];
                vid.BG0_drawAsset(curPos, tex, GetRollingFrame( m_animFrame ));
			}
            else
				vid.BG0_drawAsset(curPos, tex, 0);
			break;
		}
		case STATE_FINISHINGMOVE:
		{
            if( m_color < NUM_ANIMATED_COLORS )
            {
                const AssetImage &tex = *ANIMATEDTEXTURES[m_color];
                vid.BG0_drawAsset(vec, tex, GetRollingFrame( m_animFrame ));
            }
			break;
		}
		case STATE_MARKED:
        {
			const AssetImage &tex = GetTexture();
			if( IsFixed() )
				vid.BG0_drawAsset(vec, tex, 3);
			else
            {
                if( m_color < NUM_EXPLODING_COLORS )
                {
                    const AssetImage &exTex = GetExplodingTexture();
                    vid.BG0_drawAsset(vec, exTex, m_animFrame);
                }
                else
                    vid.BG0_drawAsset(vec, tex, 1);
            }
			break;
		}
		case STATE_EXPLODING:
		{
            if( m_color < NUM_EXPLODING_COLORS )
            {
                const AssetImage &exTex = GetExplodingTexture();
                vid.BG0_drawAsset(vec, exTex, GridSlot::NUM_EXPLODE_FRAMES - 1);
            }
            else
            {
                const AssetImage &tex = GetTexture();
                vid.BG0_drawAsset(vec, tex, 4);
            }
			break;
		}
		case STATE_SHOWINGSCORE:
		{
            if( m_score > 99 )
                m_score = 99;
            //char aStr[2];
            //sprintf( aStr, "%d", m_score );
			vid.BG0_drawAsset(vec, GemEmpty, 0);
            //vid.BG0_text(Vec2( vec.x + 1, vec.y + 1 ), Font, aStr);
            unsigned int fadeFrame = 0;

            float fadeTime = System::clock() - START_FADING_TIME - m_eventTime;

            if( fadeTime > 0.0f )
                fadeFrame =  ( fadeTime ) / FADE_FRAME_TIME;

            if( m_score > 9 )
                vid.BG0_drawAsset(Vec2( vec.x + 1, vec.y + 1 ), PointFont, m_score / 10 * NUM_POINTS_FRAMES + fadeFrame);
            vid.BG0_drawAsset(Vec2( vec.x + 2, vec.y + 1 ), PointFont, m_score % 10 * NUM_POINTS_FRAMES + fadeFrame);
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
        case STATE_LIVING:
        {
            /*if( m_pWrapper->IsIdle() )
            {
                m_animFrame++;
                if( m_animFrame >= NUM_IDLE_FRAMES )
                {
                    m_animFrame = 0;
                }
            }*/
            break;
        }
		case STATE_MOVING:
		{
			Vec2 vDiff = Vec2( m_col * 4 - m_curMovePos.x, m_row * 4 - m_curMovePos.y );

			if( vDiff.x != 0 )
				m_curMovePos.x += ( vDiff.x / abs( vDiff.x ) );
			else if( vDiff.y != 0 )
				m_curMovePos.y += ( vDiff.y / abs( vDiff.y ) );
            else if( m_color < NUM_ANIMATED_COLORS )
			{
				m_animFrame++;
                if( m_animFrame >= NUM_ROLL_FRAMES )
					m_state = STATE_LIVING;
            }
			else
				m_state = STATE_LIVING;

			break;
		}
		case STATE_FINISHINGMOVE:
		{
			m_animFrame++;
            if( m_animFrame >= NUM_ROLL_FRAMES )
				m_state = STATE_LIVING;

			break;
		}
		case STATE_MARKED:
		{
			if( t - m_eventTime > MARK_SPREAD_DELAY )
            {
                m_animFrame = ( ( t - m_eventTime ) - MARK_SPREAD_DELAY ) / EXPLODE_FRAME_LEN;
                spread_mark();
            }
            else
                m_animFrame = 0;

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
		m_animFrame = 0;
	}
}


//order of our frames
enum
{
    FRAME_REST,
    FRAME_N1,
    FRAME_N2,
    FRAME_N3,
    FRAME_NE1,
    FRAME_NE2,
    FRAME_NE3,
    FRAME_E1,
    FRAME_E2,
    FRAME_E3,
    FRAME_SE1,
    FRAME_SE2,
    FRAME_SE3,
    FRAME_S1,
    FRAME_S2,
    FRAME_S3,
    FRAME_SW1,
    FRAME_SW2,
    FRAME_SW3,
    FRAME_W1,
    FRAME_W2,
    FRAME_W3,
    FRAME_NW1,
    FRAME_NW2,
    FRAME_NW3,
};

//quantize our tilt state from [-128, 128] to [-3, 3], then offset to [0, 6]
//use those values to index into this lookup table to find which frame to render
//in row, column format
unsigned int TILTTOFRAMES[GridSlot::NUM_QUANTIZED_TILT_VALUES][GridSlot::NUM_QUANTIZED_TILT_VALUES] = {
    //most northward
    { FRAME_NW3, FRAME_NW3, FRAME_N3, FRAME_N3, FRAME_N3, FRAME_NE3, FRAME_NE3 },
    { FRAME_NW3, FRAME_NW2, FRAME_N2, FRAME_N2, FRAME_N2, FRAME_NE2, FRAME_NE3 },
    { FRAME_W3, FRAME_W2, FRAME_NW1, FRAME_N1, FRAME_NE1, FRAME_E2, FRAME_E3 },
    { FRAME_W3, FRAME_W2, FRAME_W1, FRAME_REST, FRAME_E1, FRAME_E2, FRAME_E3 },
    { FRAME_W3, FRAME_W2, FRAME_SW1, FRAME_S1, FRAME_SE1, FRAME_E2, FRAME_E3 },
    { FRAME_SW3, FRAME_SW2, FRAME_S2, FRAME_S2, FRAME_S2, FRAME_SE2, FRAME_SE3 },
    { FRAME_SW3, FRAME_SW3, FRAME_S3, FRAME_S3, FRAME_S3, FRAME_SE3, FRAME_SE3 },
};


//given tilt state, return our desired frame
unsigned int GridSlot::GetTiltFrame( Float2 &tiltState ) const
{
    //quantize and convert to the appropriate range
    //non-linear quantization.
    Vec2 quantized = Vec2( QuantizeTiltValue( tiltState.x ), QuantizeTiltValue( tiltState.y ) );

    return TILTTOFRAMES[ quantized.y ][ quantized.x ];
}



//convert from [-128, 128] to [0, 6] via non-linear quantization
unsigned int GridSlot::QuantizeTiltValue( float value ) const
{
    /*int TILT_THRESHOLD_VALUES[NUM_QUANTIZED_TILT_VALUES] =
    {
        -50,
        -30,
        -10,
        10,
        30,
        50,
        500
    };*/
    int TILT_THRESHOLD_VALUES[NUM_QUANTIZED_TILT_VALUES] =
        {
            -30,
            -20,
            -5,
            5,
            20,
            30,
            500
        };

    for( unsigned int i = 0; i < NUM_QUANTIZED_TILT_VALUES; i++ )
    {
        if( value < TILT_THRESHOLD_VALUES[i] )
            return i;
    }

    return 3;
}


static unsigned int ROLLING_FRAMES[ GridSlot::NUM_ROLL_FRAMES / GridSlot::NUM_FRAMES_PER_ROLL_ANIM_FRAME ] =
{ 1, 3, 23, 20, 17, 14, 11, 8, 10, 13, 17, 16, 0, 4, 0, 16 };

//get the rolling frame of the given index
unsigned int GridSlot::GetRollingFrame( unsigned int index )
{
    ASSERT( index < NUM_ROLL_FRAMES);

    return ROLLING_FRAMES[ index / NUM_FRAMES_PER_ROLL_ANIM_FRAME ];
}

/*
static unsigned int IDLE_FRAMES[ GridSlot::NUM_IDLE_FRAMES / GridSlot::NUM_FRAMES_PER_IDLE_ANIM_FRAME ] =
{ FRAME_N1, FRAME_E1, FRAME_S1, FRAME_W1 };


unsigned int GridSlot::GetIdleFrame()
{
    ASSERT( m_pWrapper->IsIdle() );
    if( m_animFrame >= NUM_IDLE_FRAMES )
    {
        m_animFrame = 0;
    }

    return IDLE_FRAMES[ m_animFrame / NUM_FRAMES_PER_IDLE_ANIM_FRAME ];
}
*/
