/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "GridSlot.h"
#include "game.h"
#include "assets.gen.h"
//#include "audio.gen.h"
#include "utils.h"


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

const AssetImage *GridSlot::EXPLODINGTEXTURES[ GridSlot::NUM_COLORS ] =
{
    &ExplodeGem0,
    &ExplodeGem1,
    &ExplodeGem2,
    &ExplodeGem3,
    &ExplodeGem4,
    &ExplodeGem5,
    &ExplodeGem6,
    &ExplodeGem7,
};


const AssetImage *GridSlot::FIXED_TEXTURES[ GridSlot::NUM_COLORS ] =
{
    &FixedGem0,
    &FixedGem1,
    &FixedGem2,
    &FixedGem3,
    &FixedGem4,
    &FixedGem5,
    &FixedGem6,
    &FixedGem7,
};

const AssetImage *GridSlot::FIXED_EXPLODINGTEXTURES[ GridSlot::NUM_COLORS ] =
{
    &FixedExplode0,
    &FixedExplode0,
    &FixedExplode0,
    &FixedExplode0,
    &FixedExplode0,
    &FixedExplode0,
    &FixedExplode0,
    &FixedExplode0,
};


const AssetImage *GridSlot::SPECIALTEXTURES[ NUM_SPECIALS ] =
{
    &hyperdot,
    &rockdot,
    &rainball
};



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


GridSlot::GridSlot() : 
	m_state( STATE_GONE ),
	m_eventTime( 0.0f ),
	m_score( 0 ),
	m_bFixed( false ),
	m_animFrame( 0 )
{
	m_color = Game::random.randrange(NUM_COLORS);
    m_lastFrameDir = Vec2( 0, 0 );
}


void GridSlot::Init( CubeWrapper *pWrapper, unsigned int row, unsigned int col )
{
	m_pWrapper = pWrapper;
	m_row = row;
	m_col = col;
	m_state = STATE_GONE;
    m_RockHealth = MAX_ROCK_HEALTH;
}


void GridSlot::FillColor(unsigned int color)
{
	m_state = STATE_LIVING;
	m_color = color;
	m_bFixed = false;

    if( color == ROCKCOLOR )
        m_RockHealth = MAX_ROCK_HEALTH;
}


const AssetImage &GridSlot::GetTexture() const
{
	return *TEXTURES[ m_color ];
}


const AssetImage &GridSlot::GetExplodingTexture() const
{
    if( IsFixed() )
        return *FIXED_EXPLODINGTEXTURES[ m_color ];
    else
        return *EXPLODINGTEXTURES[ m_color ];
}


const AssetImage &GridSlot::GetSpecialTexture() const
{
    return *SPECIALTEXTURES[ m_color - NUM_COLORS ];
}


unsigned int GridSlot::GetSpecialFrame() const
{
    if( m_color == ROCKCOLOR )
    {
        if( m_RockHealth > 0 )
            return MAX_ROCK_HEALTH - m_RockHealth;
        else
            return 0;
    }
    else
        return 0;
}


//draw self on given vid at given vec
void GridSlot::Draw( VidMode_BG0 &vid, Float2 &tiltState )
{
	Vec2 vec( m_col * 4, m_row * 4 );
	switch( m_state )
	{
		case STATE_LIVING:
		{
            if( IsSpecial() )
                vid.BG0_drawAsset(vec, GetSpecialTexture(), GetSpecialFrame() );
            else if( IsFixed() )
                vid.BG0_drawAsset(vec, *FIXED_TEXTURES[ m_color ]);
			else
			{
                const AssetImage &animtex = *TEXTURES[ m_color ];
                unsigned int frame;
                /*if( m_pWrapper->IsIdle() )
                    frame = GetIdleFrame();
                else*/
                    frame = GetTiltFrame( tiltState, m_lastFrameDir );
                vid.BG0_drawAsset(vec, animtex, frame);
			}
			break;
		}
		case STATE_MOVING:
		{
			Vec2 curPos = Vec2( m_curMovePos.x, m_curMovePos.y );

			//PRINT( "drawing dot x=%d, y=%d\n", m_curMovePos.x, m_curMovePos.y );
            if( IsSpecial() )
                vid.BG0_drawAsset(curPos, GetSpecialTexture(), GetSpecialFrame());
            else
            {
                const AssetImage &tex = *TEXTURES[m_color];
                vid.BG0_drawAsset(curPos, tex, GetRollingFrame( m_animFrame ));
            }
			break;
		}
		case STATE_FINISHINGMOVE:
		{           
            if( IsSpecial() )
                vid.BG0_drawAsset(vec, GetSpecialTexture(), GetSpecialFrame());
            else
            {
                const AssetImage &animtex = *TEXTURES[ m_color ];
                vid.BG0_drawAsset(vec, animtex, m_animFrame);
            }
			break;
		}
        case STATE_FIXEDATTEMPT:
        {
            vid.BG0_drawAsset(vec, *FIXED_TEXTURES[ m_color ], GetFixedFrame( m_animFrame ));
            break;
        }
		case STATE_MARKED:
        {
            if( IsSpecial() )
                vid.BG0_drawAsset(vec, GetSpecialTexture(), GetSpecialFrame() );
            else
            {
                const AssetImage &exTex = GetExplodingTexture();
                vid.BG0_drawAsset(vec, exTex, m_animFrame);
            }
			break;
		}
		case STATE_EXPLODING:
		{
            if( IsSpecial() )
                vid.BG0_drawAsset(vec, GetSpecialTexture(), GetSpecialFrame());
            else
            {
                vid.BG0_drawAsset(vec, GemEmpty, 0);
                //const AssetImage &exTex = GetExplodingTexture();
                //vid.BG0_drawAsset(vec, exTex, GridSlot::NUM_EXPLODE_FRAMES - 1);
            }
			break;
		}
		case STATE_SHOWINGSCORE:
		{
            if( m_score > 99 )
                m_score = 99;
			vid.BG0_drawAsset(vec, GemEmpty, 0);
            unsigned int fadeFrame = 0;

            float fadeTime = System::clock() - START_FADING_TIME - m_eventTime;

            if( fadeTime > 0.0f )
                fadeFrame =  ( fadeTime ) / FADE_FRAME_TIME;

            if( fadeFrame >= NUM_POINTS_FRAMES )
                fadeFrame = NUM_POINTS_FRAMES - 1;

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
            {
				m_curMovePos.x += ( vDiff.x / abs( vDiff.x ) );
                //clear this out in update
                m_pWrapper->QueueClear( m_curMovePos );

                if( abs( vDiff.x ) == 1 )
                    Game::Inst().playSound(collide_02);
            }
			else if( vDiff.y != 0 )
            {
				m_curMovePos.y += ( vDiff.y / abs( vDiff.y ) );
                //clear this out in update
                m_pWrapper->QueueClear( m_curMovePos );

                if( abs( vDiff.y ) == 1 )
                    Game::Inst().playSound(collide_02);
            }
            else
			{
				m_animFrame++;
                if( m_animFrame >= NUM_ROLL_FRAMES )
                {
                    m_state = STATE_FINISHINGMOVE;
                    m_animFrame = GetRollingFrame( NUM_ROLL_FRAMES - 1 );
                }
            }

			break;
		}
		case STATE_FINISHINGMOVE:
		{
            //interpolate frames back to normal state
            Float2 cubeDir = m_pWrapper->getTiltDir();
            Vec2 curDir;

            GetTiltFrame( cubeDir, curDir );

            if( m_lastFrameDir == curDir )
                m_state = STATE_LIVING;
            else
            {
                if( curDir.x > m_lastFrameDir.x )
                    m_lastFrameDir.x++;
                else if( curDir.x < m_lastFrameDir.x )
                    m_lastFrameDir.x--;
                if( curDir.y > m_lastFrameDir.y )
                    m_lastFrameDir.y++;
                else if( curDir.y < m_lastFrameDir.y )
                    m_lastFrameDir.y--;

                m_animFrame = TILTTOFRAMES[ m_lastFrameDir.y ][ m_lastFrameDir.x ];
            }

			break;
		}
        case STATE_FIXEDATTEMPT:
        {
            ASSERT( IsFixed() );
            m_animFrame++;
            if( m_animFrame / NUM_FRAMES_PER_FIXED_ANIM_FRAME >= NUM_FIXED_FRAMES )
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
            {
                explode();
                //Game::Inst().playSound(bubble_pop_02);
            }
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
        //clear this out in update, so it doesn't bash moving balls
        case STATE_GONE:
        {
            Vec2 vec( m_col * 4, m_row * 4 );
            m_pWrapper->QueueClear( vec );
            //vid.BG0_drawAsset(vec, GemEmpty, 0);
            break;
        }
		default:
			break;
	}
}


void GridSlot::mark()
{
    if( m_state == STATE_MARKED || m_state == STATE_EXPLODING )
        return;
    m_animFrame = 0;
	m_state = STATE_MARKED;
	m_eventTime = System::clock();
    Game::Inst().playSound(match2);

    if( m_color < NUM_COLORS )
        Game::Inst().SetUsedColor( m_color );
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

    hurtNeighboringRock( m_row - 1, m_col );
    hurtNeighboringRock( m_row, m_col - 1 );
    hurtNeighboringRock( m_row + 1, m_col );
    hurtNeighboringRock( m_row, m_col + 1 );

	m_eventTime = System::clock();
}

void GridSlot::die()
{
	m_state = STATE_SHOWINGSCORE;
    m_bFixed = false;
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
    if( pNeighbor && pNeighbor->isMatchable() && !pNeighbor->isMarked() && pNeighbor->getColor() == m_color )
		pNeighbor->mark();
}


void GridSlot::hurtNeighboringRock( int row, int col )
{
    //find my neighbor and see if we match
    GridSlot *pNeighbor = m_pWrapper->GetSlot( row, col );

    //PRINT( "pneighbor = %p", pNeighbor );
    //PRINT( "color = %d", pNeighbor->getColor() );
    if( pNeighbor && pNeighbor->getColor() == ROCKCOLOR )
        pNeighbor->DamageRock();
}


void GridSlot::DamageRock()
{
    if( m_RockHealth > 0 )
    {
        m_RockHealth--;

        if( m_RockHealth == 0 )
            explode();
    }
}



//copy color and some other attributes from target.  Used when tilting
void GridSlot::TiltFrom(GridSlot &src)
{
    m_bFixed = false;
	m_state = STATE_PENDINGMOVE;
	m_color = src.m_color;
	m_eventTime = src.m_eventTime;
	m_curMovePos.x = src.m_col * 4;
	m_curMovePos.y = src.m_row * 4;
    m_RockHealth = src.m_RockHealth;
}


//if we have a move pending, start it
void GridSlot::startPendingMove()
{
	if( m_state == STATE_PENDINGMOVE )
	{
        Game::Inst().playSound(slide_39);
		m_state = STATE_MOVING;
		m_animFrame = 0;
	}
}


//fake moves don't animate or take time, just finish them
void GridSlot::finishFakeMove()
{
    if( m_state == STATE_PENDINGMOVE )
    {
        m_state = STATE_LIVING;
    }
}


//given tilt state, return our desired frame
unsigned int GridSlot::GetTiltFrame( Float2 &tiltState, Vec2 &quantized ) const
{
    //quantize and convert to the appropriate range
    //non-linear quantization.
    quantized = Vec2( QuantizeTiltValue( tiltState.x ), QuantizeTiltValue( tiltState.y ) );

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


unsigned int GridSlot::GetFixedFrame( unsigned int index )
{
    ASSERT( index / NUM_FRAMES_PER_FIXED_ANIM_FRAME < NUM_FIXED_FRAMES);

    int frame = NUM_FIXED_FRAMES * m_pWrapper->getLastTiltDir() + ( index / NUM_FRAMES_PER_FIXED_ANIM_FRAME ) + 1;

    return frame;
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


void GridSlot::DrawIntroFrame( VidMode_BG0 &vid, unsigned int frame )
{
    Vec2 vec( m_col * 4, m_row * 4 );

    if( IsSpecial() )
        vid.BG0_drawAsset(vec, GetSpecialTexture(), GetSpecialFrame());
    else
    {
        switch( frame )
        {
            case 0:
            {
                vid.BG0_drawAsset(vec, GemEmpty, 0);
                break;
            }
            case 1:
            {
                const AssetImage &exTex = GetExplodingTexture();
                vid.BG0_drawAsset(vec, exTex, 1);
                break;
            }
            case 2:
            {
                const AssetImage &exTex = GetExplodingTexture();
                vid.BG0_drawAsset(vec, exTex, 0);
                break;
            }
            default:
            {
                const AssetImage &tex = *TEXTURES[ m_color ];
                vid.BG0_drawAsset(vec, tex, 0);
                break;
            }
        }
    }
}


void GridSlot::setFixedAttempt()
{
    m_state = STATE_FIXEDATTEMPT;
    m_animFrame = 0;
    Game::Inst().playSound(frozen_06);
}
