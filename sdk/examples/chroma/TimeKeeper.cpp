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
const float TimeKeeper::TIMER_SPRITE_PERIOD = 2.0f;
const float TimeKeeper::TIMER_LOW_SPRITE_PERIOD = 0.3f;


TimeKeeper::TimeKeeper()
{
    Reset();
}


void TimeKeeper::Reset()
{
	m_fTimer = TIME_INITIAL;
    //m_blinkCounter = 0;
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
    //m_blinkCounter++;
}


void TimeKeeper::Init( float t )
{
	Reset();
}


void TimeKeeper::DrawMeter( float amount, BG1Helper &bg1helper, VidMode_BG0_SPR_BG1 &vid )
{
    if( amount > 1.0f )
        amount = 1.0f;

    int numStems = TIMER_STEMS * amount;

    /*if( numStems <= 2 && m_blinkCounter < BLINK_OFF_FRAMES )
    {
        vid.resizeSprite(0, 0, 0);
        return;
    }*/

    if( numStems <= 2 )
    {
        float spritePerc = 1.0f - Math::fmodf( m_fTimer, TIMER_LOW_SPRITE_PERIOD ) / TIMER_LOW_SPRITE_PERIOD;
        unsigned int spriteframe = spritePerc * ( timerLow.frames + 1 );

        if( spriteframe >= timerLow.frames )
            spriteframe = timerLow.frames - 1;

        vid.resizeSprite(0, timerLow.width*8, timerLow.height*8);
        vid.setSpriteImage(0, timerLow, spriteframe);
        vid.moveSprite(0, TIMER_SPRITE_POS, TIMER_SPRITE_POS);
    }
    else
    {
        //figure out what frame we're on
        float spritePerc = 1.0f - Math::fmodf( m_fTimer, TIMER_SPRITE_PERIOD ) / TIMER_SPRITE_PERIOD;
        unsigned int spriteframe = spritePerc * ( timerSprite.frames + 1 );

        if( spriteframe >= timerSprite.frames )
            spriteframe = timerSprite.frames - 1;

        vid.resizeSprite(0, timerSprite.width*8, timerSprite.height*8);
        vid.setSpriteImage(0, timerSprite, spriteframe);
        vid.moveSprite(0, TIMER_SPRITE_POS, TIMER_SPRITE_POS);
    }


    if( numStems > 0 )
    {
        bg1helper.DrawAsset( Vec2( TIMER_POS, TIMER_POS ), timerStem, TIMER_STEMS - numStems );
    }

    /*if( numStems <= 2 && m_blinkCounter - BLINK_OFF_FRAMES >= BLINK_ON_FRAMES )
    {
        m_blinkCounter = 0;
        Game::Inst().playSound(timer_blink);
    }*/
}
