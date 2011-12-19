/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * timer for the game
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "Intro.h"
#include "string.h"
#include "assets.gen.h"
#include "audio.gen.h"
#include "sprite.h"
#include "game.h"

const float Intro::INTRO_ARROW_TIME = 0.4f;
const float Intro::INTRO_TIMEREXPANSION_TIME = 0.5f;
const float Intro::INTRO_BALLEXPLODE_TIME = 0.5f;


Intro::Intro()
{
    m_fTimer = 0.0f;
}


void Intro::Reset()
{
    m_fTimer = 0.0f;
}


void Intro::Update( float dt )
{
    if( m_fTimer == 0.0f )
        Game::Inst().playSound(glom_delay);

    m_fTimer += dt;

    if( m_fTimer > INTRO_ARROW_TIME + INTRO_TIMEREXPANSION_TIME + INTRO_BALLEXPLODE_TIME )
        Game::Inst().setState( Game::STATE_PLAYING );
}

const Sifteo::PinnedAssetImage *ARROW_SPRITES[ Intro::NUM_ARROWS ] =
{
    &ArrowUp,
    &ArrowLeft,
    &ArrowDown,
    &ArrowRight,
};

Vec2 STARTPOS[ Intro::NUM_ARROWS ] = {
    Vec2( 64 - ArrowUp.width * 8 / 2, 64 - ArrowUp.height * 8 / 2 ),
    Vec2( 64 - ArrowLeft.width * 8 / 2, 64 - ArrowLeft.height * 8 / 2 ),
    Vec2( 64 - ArrowDown.width * 8 / 2, 64 - ArrowDown.height * 8 / 2 ),
    Vec2( 64 - ArrowRight.width * 8 / 2, 64 - ArrowRight.height * 8 / 2 ),
};


Vec2 ENDPOS[ Intro::NUM_ARROWS ] = {
    Vec2( 64 - ArrowUp.width * 8 /2 , 0 ),
    Vec2( 0, 64 - ArrowLeft.height * 8 / 2  ),
    Vec2( 64 - ArrowDown.width * 8 / 2, 128 - ArrowDown.height * 8 ),
    Vec2( 128 - ArrowRight.width * 8, 64 - ArrowRight.height * 8 / 2 ),
};

void Intro::Draw( TimeKeeper &timer, BG1Helper &bg1helper, Cube &cube, CubeWrapper *pWrapper )
{
    VidMode_BG0 vid( cube.vbuf );
    _SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);
    vid.clear(GemEmpty.tiles[0]);

    if( m_fTimer < INTRO_ARROW_TIME )
    {
        float timePercent = m_fTimer / INTRO_ARROW_TIME;

        //arrow sprites
        for( int i = 0; i < NUM_ARROWS; i++ )
        {
            Vec2 pos = LerpPosition( STARTPOS[i], ENDPOS[i], timePercent );
            resizeSprite(cube, i, ARROW_SPRITES[i]->width*8, ARROW_SPRITES[i]->height*8);
            setSpriteImage(cube, i, *ARROW_SPRITES[i], 0);
            moveSprite(cube, i, pos.x, pos.y);
        }
    }
    else if( m_fTimer < INTRO_ARROW_TIME + INTRO_TIMEREXPANSION_TIME )
    {
        for( int i = 0; i < NUM_ARROWS; i++ )
        {
            resizeSprite(cube, i, 0, 0);
        }

        //charge up timers
        float amount = ( m_fTimer - INTRO_ARROW_TIME ) / INTRO_TIMEREXPANSION_TIME;
        timer.DrawMeter( amount, bg1helper );
        bg1helper.Flush();
    }
    else
    {
        int baseFrame = ( m_fTimer - ( INTRO_ARROW_TIME + INTRO_TIMEREXPANSION_TIME ) ) / INTRO_BALLEXPLODE_TIME * NUM_TOTAL_EXPLOSION_FRAMES;

        for( int i = 0; i < CubeWrapper::NUM_ROWS; i++ )
        {
            for( int j = 0; j < CubeWrapper::NUM_COLS; j++ )
            {
                GridSlot *pSlot = pWrapper->GetSlot( i, j );
                int frame = baseFrame;

                //middle ones are first
                if( i >= 1 && i < 3 && j >= 1 && j < 3 )
                    frame++;
                //corner ones are last
                if( frame > 0 && ( i == 0 || i == 3 ) && ( j == 0 || j == 3 ) )
                    frame--;

                pSlot->DrawIntroFrame( vid, frame );
            }
        }
    }
}



Vec2 Intro::LerpPosition( Vec2 &start, Vec2 &end, float timePercent )
{
    Vec2 result( start.x + ( end.x - start.x ) * timePercent, start.y + ( end.y - start.y ) * timePercent );

    return result;
}
