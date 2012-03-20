/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * timer for the game
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "Intro.h"
#include "assets.gen.h"
//#include "audio.gen.h"
#include "game.h"

const float Intro::READYSETGO_BANNER_TIME = 0.9f;

const float STATE_TIMES[ Intro::STATE_CNT ] = {
    //STATE_BALLEXPLOSION,
    0.5f,
    //STATE_READY,
    1.0f,
    //STATE_SET,
    1.0f,
    //STATE_GO,
    1.0f,
};


Intro::Intro()
{
    m_fTimer = 0.0f;
    m_state = STATE_BALLEXPLOSION;
}


void Intro::Reset( bool ingamereset)
{
    m_state = STATE_BALLEXPLOSION;
    if( ingamereset )
    {
        Game::Inst().playSound(glom_delay);
    }

    m_fTimer = 0.0f;
}


bool Intro::Update( SystemTime t, TimeDelta dt, Banner &banner )
{
    m_fTimer += dt;

    if( m_fTimer >= STATE_TIMES[ m_state ] )
    {
        m_state = (IntroState)( (int)m_state + 1 );
        m_fTimer = 0.0f;

        //special cases
        switch( m_state )
        {
            case STATE_BALLEXPLOSION:
                Game::Inst().playSound(glom_delay);
                break;
            case STATE_READY:
                if( Game::Inst().getState() == Game::STATE_PLAYING )
                {
                    return false;
                }
                else if( Game::Inst().getMode() == Game::MODE_BLITZ )
                    banner.SetMessage( "60 seconds", READYSETGO_BANNER_TIME );
                else if( Game::Inst().getMode() == Game::MODE_PUZZLE )
                {
                    Game::Inst().setState( Game::STATE_PLAYING );
                    return false;
                }
                else
                    banner.SetMessage( "Clear the cubes!", READYSETGO_BANNER_TIME );
                break;
            case STATE_SET:
                if( Game::Inst().getMode() == Game::MODE_BLITZ )
                    banner.SetMessage( "Ready", READYSETGO_BANNER_TIME );
                else
                    return false;
                break;
            case STATE_GO:
                banner.SetMessage( "Go!", READYSETGO_BANNER_TIME );
                break;
            case STATE_CNT:
                Game::Inst().setState( Game::STATE_PLAYING );
                return false;
            default:
                break;
        }
    }

    banner.Update( t );

    return true;
}

//return whether we touched bg1 or not
bool Intro::Draw( TimeKeeper &timer, BG1Helper &bg1helper, VidMode_BG0_SPR_BG1 &vid, CubeWrapper *pWrapper )
{
    float timePercent = m_fTimer / STATE_TIMES[ m_state ];

    switch( m_state )
    {
        case STATE_BALLEXPLOSION:
        {
            int baseFrame = timePercent * NUM_TOTAL_EXPLOSION_FRAMES;

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
            break;
        }
        default:
            pWrapper->getBanner().Draw( bg1helper );
            return true;
    }

    return false;
}



Vec2 Intro::LerpPosition( Vec2 &start, Vec2 &end, float timePercent )
{
    Vec2 result( start.x + ( end.x - start.x ) * timePercent, start.y + ( end.y - start.y ) * timePercent );

    return result;
}
