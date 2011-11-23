/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * timer for the game
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "TimeKeeper.h"
#include "string.h"
#include "assets.gen.h"

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

	if( fTimerProportion > 1.0f )
		fTimerProportion = 1.0f;

	int numTiles = TIMER_TILES * fTimerProportion;

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
        int numSubTiles = ( ( TIMER_TILES * fTimerProportion ) - numTiles ) * TIMER_TILES;

        bg1helper.DrawAsset( Vec2( offset - 1, 0 ), TimerEdgeUpLeft, numSubTiles );
        bg1helper.DrawAsset( Vec2( ( TIMER_TILES * 2 ) - ( offset ), 0 ), TimerEdgeUpRight, numSubTiles );
    }

/*
	//for now, just draw in the corner
	_SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);
	// Allocate tiles for the timer

	char aBuf[16];
	//how many digits
	int iTimer = (int)m_fTimer;

	sprintf( aBuf, "%d", iTimer );
	int iDigits = strlen( aBuf );

	switch( iDigits )
	{
		case 1:
			_SYS_vbuf_fill(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2, 0x8000, 2 );
			break;
		case 2:
			_SYS_vbuf_fill(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2, 0xC000, 2 );
			break;
		case 3:
			_SYS_vbuf_fill(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2, 0xE000, 2 );
			break;
	}
	

	//draw timer
	for( int i = 0; i < iDigits; i++ )
	{
		//double tall
		for( int j = 0; j < 2; j++ )
		{
			_SYS_vbuf_writei(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2 + i + ( iDigits * ( j ) ),
							 Font.tiles + ( ( aBuf[i] - ' ' ) * 2 ) + j,
							 0, 1);
		}
	}

	_SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_y), 0);
	*/
}


void TimeKeeper::Update(float dt)
{
	m_fTimer -= dt;
}


void TimeKeeper::Init( float t )
{
	Reset();
}


void TimeKeeper::AddTime( int numGems )
{
	m_fTimer += numGems * TIME_RETURN_PER_GEM;
}
