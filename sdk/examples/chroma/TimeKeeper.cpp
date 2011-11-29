/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * timer for the game
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "TimeKeeper.h"
#include "string.h"
#include "assets.gen.h"

const float TimeKeeper::TIME_INITIAL = 60.0f;
const float TimeKeeper::TIME_RETURN_PER_GEM = 1.0f;


TimeKeeper::TimeKeeper()
{
	m_fTimer = TIME_INITIAL;
}


void TimeKeeper::Reset()
{
	m_fTimer = TIME_INITIAL;
}

void TimeKeeper::Draw( BG1Helper &bg1helper )
{
	//find out what proportion of our timer is left, then multiply by number of tiles
	float fTimerProportion = m_fTimer / TIME_INITIAL;

    DrawMeter( fTimerProportion, bg1helper );
}


void TimeKeeper::Update(float dt)
{
	m_fTimer -= dt;
}


void TimeKeeper::Init( float t )
{
	Reset();
}


void TimeKeeper::DrawMeter( float amount, BG1Helper &bg1helper )
{
    if( amount > 1.0f )
        amount = 1.0f;

    int numTiles = TIMER_TILES * amount;

    //have one more
    /*numTiles++;
    if( numTiles > TIMER_TILES )
        numTiles = TIMER_TILES;*/

    int offset = TIMER_TILES - numTiles;

    if( numTiles > 0 )
    {
        bg1helper.DrawPartialAsset( Vec2(offset,0), Vec2(offset,0), Vec2(numTiles * 2,TimerUp.height), TimerUp );
        bg1helper.DrawPartialAsset( Vec2(0,offset), Vec2(0,offset), Vec2(TimerLeft.width,numTiles * 2), TimerLeft );
        bg1helper.DrawPartialAsset( Vec2(offset,15), Vec2(offset,0), Vec2(numTiles * 2,TimerDown.height), TimerDown );
        bg1helper.DrawPartialAsset( Vec2(15,offset), Vec2(0,offset), Vec2(TimerRight.width,numTiles * 2), TimerRight );
    }

    if( numTiles < TIMER_TILES )
    {
        //draw partial timer bits
        int numSubTiles = ( ( TIMER_TILES * amount ) - numTiles ) * TIMER_TILES;

        bg1helper.DrawAsset( Vec2( offset - 1, 0 ), TimerEdgeUpLeft, numSubTiles );
        bg1helper.DrawAsset( Vec2( ( TIMER_TILES * 2 ) - ( offset ), 0 ), TimerEdgeUpRight, numSubTiles );
    }
}


void TimeKeeper::AddTime( int numGems )
{
	m_fTimer += numGems * TIME_RETURN_PER_GEM;
}
