/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * handles glimmering balls on idle
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "Glimmer.h"
//#include "string.h"
#include "assets.gen.h"
//#include "sprite.h"
#include "game.h"
#include "cubewrapper.h"


Vec2 GLIMMER_ORDER_1[] = { Vec2( 0, 0 ) };
Vec2 GLIMMER_ORDER_2[] = { Vec2( 1, 0 ), Vec2( 0, 1 ) };
Vec2 GLIMMER_ORDER_3[] = { Vec2( 2, 0 ), Vec2( 1, 1 ), Vec2( 0, 2 ) };
Vec2 GLIMMER_ORDER_4[] = { Vec2( 3, 0 ), Vec2( 1, 2 ), Vec2( 2, 1 ), Vec2( 0, 3 ) };
Vec2 GLIMMER_ORDER_5[] = { Vec2( 3, 1 ), Vec2( 2, 2 ), Vec2( 1, 3 ) };
Vec2 GLIMMER_ORDER_6[] = { Vec2( 3, 2 ), Vec2( 2, 3 ) };
Vec2 GLIMMER_ORDER_7[] = { Vec2( 3, 3 ) };

//list of locations to glimmer in order
Vec2 *Glimmer::GLIMMER_ORDER[NUM_GLIMMER_GROUPS] =
{
    GLIMMER_ORDER_1,
    GLIMMER_ORDER_2,
    GLIMMER_ORDER_3,
    GLIMMER_ORDER_4,
    GLIMMER_ORDER_5,
    GLIMMER_ORDER_6,
    GLIMMER_ORDER_7,
};


Glimmer::Glimmer()
{
    Reset();
}


void Glimmer::Reset()
{
    m_frame = 0;
    m_group = 0;
}


void Glimmer::Update( float dt )
{
    if( m_group < NUM_GLIMMER_GROUPS )
    {
        m_frame++;

        if( m_frame >= GlimmerImg.frames )
        {
            m_frame = 0;
            m_group++;
        }
    }
}


int Glimmer::NUM_PER_GROUP[NUM_GLIMMER_GROUPS] =
{
  1,
    2,
    3,
    4,
    3,
    2,
    1
};


void Glimmer::Draw( BG1Helper &bg1helper, CubeWrapper *pWrapper )
{
    //old sprite version
    /*if( m_group >= NUM_GLIMMER_GROUPS )
    {
        for( int i = 0; i < MAX_GLIMMERS; i++ )
            resizeSprite(cube, i, 0, 0);
        return;
    }

    for( int i = 0; i < MAX_GLIMMERS; i++ )
    {
        if( i < NUM_PER_GROUP[ m_group ] )
        {
            Vec2 &loc = GLIMMER_ORDER[ m_group ][i];
            resizeSprite(cube, i, GlimmerImg.width*8, GlimmerImg.height*8);
            moveSprite(cube, i, loc.x * 32, loc.y * 32);
        }
        else
            resizeSprite(cube, i, 0, 0);
        setSpriteImage(cube, i, GlimmerImg, m_frame);

    }*/

    //bg1 version
    if( m_group >= NUM_GLIMMER_GROUPS )
    {
        return;
    }

    for( int i = 0; i < MAX_GLIMMERS; i++ )
    {
        if( i < NUM_PER_GROUP[ m_group ] )
        {
            Vec2 &loc = GLIMMER_ORDER[ m_group ][i];
            if( pWrapper->GetSlot( loc.x, loc.y )->isAlive() )
                bg1helper.DrawAsset( Vec2( loc.y * 4, loc.x * 4 ), GlimmerImg, m_frame );
        }
    }
}


