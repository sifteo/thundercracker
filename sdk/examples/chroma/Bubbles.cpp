/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * handles bubble sprites
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "Bubbles.h"
#include "Cubewrapper.h"
#include "game.h"
#include "GridSlot.h"
#include "assets.gen.h"

//min/max for short period
const float BubbleSpawner::MIN_SHORT_SPAWN = 0.1f;
const float BubbleSpawner::MAX_SHORT_SPAWN = 0.6f;
//min/max for long period
const float BubbleSpawner::MIN_LONG_SPAWN = 6.0f;
const float BubbleSpawner::MAX_LONG_SPAWN = 8.0f;
//chance that we do a short period
const float BubbleSpawner::SHORT_PERIOD_CHANCE = 0.75f;


BubbleSpawner::BubbleSpawner( VidMode_BG0_SPR_BG1 &vid )
{
    Reset( vid );
}


void BubbleSpawner::Reset( VidMode_BG0_SPR_BG1 &vid )
{
    m_fTimeTillSpawn = Game::random.uniform( MAX_SHORT_SPAWN, MIN_LONG_SPAWN );

    for( unsigned int i = 0; i < MAX_BUBBLES; i++ )
    {
        m_aBubbles[i].Disable();
        vid.resizeSprite(BUBBLE_SPRITEINDEX + i, 0, 0);
    }
}


void BubbleSpawner::Update( float dt, const Float2 &tilt )
{
    int slotAvailable = -1;

    for( unsigned int i = 0; i < MAX_BUBBLES; i++ )
    {
        if( m_aBubbles[i].isAlive() )
            m_aBubbles[i].Update( dt, tilt );
        else if( slotAvailable < 0 )
            slotAvailable = i;
    }

    if( slotAvailable >= 0 )
    {
        m_fTimeTillSpawn -= dt;

        if( m_fTimeTillSpawn < 0.0f )
        {
            if( Game::random.random() < SHORT_PERIOD_CHANCE )
                m_fTimeTillSpawn = Game::random.uniform( MIN_SHORT_SPAWN, MAX_SHORT_SPAWN );
            else
                m_fTimeTillSpawn = Game::random.uniform( MIN_LONG_SPAWN, MAX_LONG_SPAWN );
            m_aBubbles[slotAvailable].Spawn();
        }
    }
}



void BubbleSpawner::Draw( VidMode_BG0_SPR_BG1 &vid, CubeWrapper *pWrapper )
{
    for( unsigned int i = 0; i < MAX_BUBBLES; i++ )
    {
        unsigned int spriteindex = BUBBLE_SPRITEINDEX + i;
        if( m_aBubbles[i].isAlive() )
            m_aBubbles[i].Draw( vid, spriteindex, pWrapper );
        else
            vid.resizeSprite(spriteindex, 0, 0);
    }
}


const float Bubble::BUBBLE_LIFETIME = 2.5f;
const float Bubble::TILT_VEL = 128.0f;
const float Bubble::BEHIND_CHROMITS_THRESHOLD = 0.9f;
//at this depth, bubbles will move away from chromits
const float Bubble::CHROMITS_COLLISION_DEPTH = BEHIND_CHROMITS_THRESHOLD * 0.8888888f;
const float Bubble::CHROMIT_OBSCURE_DIST = 12.25f;
const float Bubble::CHROMIT_OBSCURE_DIST_2 = CHROMIT_OBSCURE_DIST * CHROMIT_OBSCURE_DIST;

Bubble::Bubble() : m_fTimeAlive( -1.0f )
{
}

void Bubble::Spawn()
{
    m_pos.set( 64.0f + Game::random.uniform( -SPAWN_EXTENT, SPAWN_EXTENT ), 64.0f + Game::random.uniform( -SPAWN_EXTENT, SPAWN_EXTENT ) );
    m_fTimeAlive = 0.0f;
}

void Bubble::Disable()
{
    m_fTimeAlive = -1.0f;
}

void Bubble::Update( float dt, const Float2 &tilt )
{
    m_fTimeAlive += dt;

    m_pos.x -= tilt.x / 64.0f * TILT_VEL * dt;
    m_pos.y -= tilt.y / 64.0f * TILT_VEL * dt;

    if( m_fTimeAlive >= BUBBLE_LIFETIME )
        Disable();
}

void Bubble::Draw( VidMode_BG0_SPR_BG1 &vid, int index, CubeWrapper *pWrapper )
{
    unsigned int frame = m_fTimeAlive / BUBBLE_LIFETIME * bubbles.frames;
    bool visible = true;

    if( frame >= bubbles.frames )
        frame = bubbles.frames - 1;

    //sometimes bubbles are obscured by chromits
    if( m_fTimeAlive / BUBBLE_LIFETIME < BEHIND_CHROMITS_THRESHOLD )
    {
        //find our center
        Float2 center( m_pos.x + bubbles.width*4, m_pos.y + bubbles.height*4 );
        Vec2 gridslot( center.x / 32, center.y / 32 );

        GridSlot *pSlot = pWrapper->GetSlot( gridslot.y, gridslot.x );

        if( pSlot && !pSlot->isEmpty() )
        {
            //if our center is inside a certain radius, we are hidden
            Float2 slotcenter( gridslot.x * 32 + 16, gridslot.y * 32 + 16 );
            Float2 diff = center - slotcenter;

            if( diff.len2() < CHROMIT_OBSCURE_DIST_2 )
            {
                visible = false;

                //move away from chromit
                if( m_fTimeAlive / BUBBLE_LIFETIME > CHROMITS_COLLISION_DEPTH )
                {
                    NormalizeVec( diff );
                    m_pos += ( diff * CHROMIT_OBSCURE_DIST );

                    //bump the chromit
                    pSlot->Bump( -diff );
                }
            }
        }
    }

    if( visible )
    {
        vid.resizeSprite(index, bubbles.width*8, bubbles.height*8);
        vid.setSpriteImage(index, bubbles, frame);
        vid.moveSprite(index, m_pos.x, m_pos.y);
    }
    else
        vid.resizeSprite(index, 0, 0);
}

//TODO, new sdk has normalize
//voodoo black magic, from sources long lost:
//http://www.beyond3d.com/content/articles/8/
float InvSqrt (float x)
{
    float xhalf = 0.5f*x;
    int i = *(int*)&x;
    i = 0x5f3759df - (i>>1);
    x = *(float*)&i;
    x = x*(1.5f - xhalf*x*x);
    return x;
}

//normalize the given vector
void Bubble::NormalizeVec( Float2 &vec )
{
    vec *= InvSqrt( vec.len2() );
}
