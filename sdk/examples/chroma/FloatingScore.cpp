/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * handles drawing of score floating up
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "FloatingScore.h"
#include "Banner.h"
#include "assets.gen.h"

const float FloatingScore::SCORE_FADE_DELAY = 2.0f;
//const float FloatingScore::START_FADING_TIME = 0.25f;

FloatingScore::FloatingScore()
{
    Reset();
}

void FloatingScore::Reset()
{
    m_fLifetime = -1.0f;
}

void FloatingScore::Draw( BG1Helper &bg1helper )
{
    if( m_fLifetime < 0.0f )
        return;

    /*int frame = 0;

    if( m_fLifetime < START_FADING_TIME )
        frame = ( START_FADING_TIME - m_fLifetime ) / START_FADING_TIME * NUM_POINTS_FRAMES;

    Banner::DrawScore( bg1helper, m_pos, Banner::LEFT, m_score, frame );*/
    String<16> buf;
    buf << m_score;

    int iLen = buf.size();
    if( iLen == 0 )
        return;

    unsigned int xOff = ( 16 - ( iLen * PointFont.width ) ) / 2;
    unsigned int yOff = ( 16 - ( PointFont.height ) ) / 2;

    for( int i = 0; i < iLen; i++ )
    {
        bg1helper.DrawAsset( Vec2( xOff + ( i * PointFont.width ), yOff ), PointFont, buf[i] - '0' );
    }
}


void FloatingScore::Update(float dt)
{
    m_fLifetime -= dt;
}


void FloatingScore::Spawn( unsigned int score, const Int2 &pos )
{
    m_score = score;
    m_pos = pos;
    m_fLifetime = SCORE_FADE_DELAY;
}
