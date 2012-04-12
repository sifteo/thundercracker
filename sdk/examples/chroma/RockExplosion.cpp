/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Logic for handling rock explosions
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "assets.gen.h"
#include "RockExplosion.h"


const float RockExplosion::FRAMES_PER_SECOND = 12.0f;


RockExplosion::RockExplosion()
{
    Reset();
}


void RockExplosion::Reset()
{
    m_pos.set( -1, -1 );
    m_animFrame = 0;
}


void RockExplosion::Update()
{
    if( m_pos.x >= 0 )
    {
        m_animFrame = float(SystemTime::now() - m_startTime) * FRAMES_PER_SECOND;

        if( m_animFrame >= rock_explode.frames )
        {
            Reset();
        }
    }
}


void RockExplosion::Draw( VidMode_BG0_SPR_BG1 &vid, int spriteindex )
{
    if( m_pos.x >= 0 )
    {
        vid.resizeSprite(spriteindex, rock_explode.width*8, rock_explode.height*8);
        vid.setSpriteImage(spriteindex, rock_explode, m_animFrame);
        vid.moveSprite(spriteindex, m_pos.x, m_pos.y);
    }
    else
        vid.resizeSprite(spriteindex, 0, 0);
}


static const Int2 OFFSET[] =
{
    vec( -8, 8 ),
    vec( 8, 8 ),
    vec( 8, -8 ),
    vec( -8, -8 ),
};


void RockExplosion::Spawn( const Int2 &pos, int whichpiece )
{
    m_pos.set( pos.x * 8 + OFFSET[whichpiece].x, pos.y * 8 + OFFSET[whichpiece].y );
    m_animFrame = 0;
    m_startTime = SystemTime::now();
}
