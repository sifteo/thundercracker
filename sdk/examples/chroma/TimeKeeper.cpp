/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * timer for the game
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "TimeKeeper.h"
#include "string.h"
#include "Game.h"
#include "assets.gen.h"
//#include "audio.gen.h"

const float TimeKeeper::TIME_INITIAL = 60.0f;


TimeKeeper::TimeKeeper()
{
    Reset();
}


void TimeKeeper::Reset()
{
	m_fTimer = TIME_INITIAL;
    m_blinkCounter = 0;
    m_timerFrame = 0;
}

void TimeKeeper::Draw( BG1Helper &bg1helper, VidMode_BG0_SPR_BG1 &vid )
{
	//find out what proportion of our timer is left, then multiply by number of tiles
	float fTimerProportion = m_fTimer / TIME_INITIAL;

    DrawMeter( fTimerProportion, bg1helper, vid );
}


void TimeKeeper::Update(float dt)
{
	m_fTimer -= dt;
    m_blinkCounter++;
}


void TimeKeeper::Init( float t )
{
	Reset();
}


void TimeKeeper::DrawMeter( float amount, BG1Helper &bg1helper, VidMode_BG0_SPR_BG1 &vid )
{
    if( amount > 1.0f )
        amount = 1.0f;

    //always draw the sprite component
    vid.resizeSprite(0, timerSprite.width*8, timerSprite.height*8);
    vid.setSpriteImage(0, timerSprite, m_timerFrame);
    vid.moveSprite(0, TIMER_SPRITE_POS, TIMER_SPRITE_POS);

    m_timerFrame++;

    if( m_timerFrame >= timerSprite.frames )
        m_timerFrame = 0;

    int numStems = TIMER_STEMS * amount;

    if( numStems > 0 )
    {
        if( numStems > 2 )
            bg1helper.DrawAsset( Vec2( TIMER_POS, TIMER_POS ), timerStem, TIMER_STEMS - numStems );
        else if( m_blinkCounter >= BLINK_OFF_FRAMES )
        {
            bg1helper.DrawAsset( Vec2( TIMER_POS, TIMER_POS ), timerStem, TIMER_STEMS - numStems );

            if( m_blinkCounter - BLINK_OFF_FRAMES >= BLINK_ON_FRAMES )
            {
                m_blinkCounter = 0;
                Game::Inst().playSound(timer_blink);
            }
        }
    }
}
