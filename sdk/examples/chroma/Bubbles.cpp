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


BubbleSpawner::BubbleSpawner( VideoBuffer &vid )
{
    Reset( vid );
}


void BubbleSpawner::Reset( VideoBuffer &vid )
{
    m_fTimeTillSpawn = Game::random.uniform( MAX_SHORT_SPAWN, MIN_LONG_SPAWN );

    for( unsigned int i = 0; i < MAX_BUBBLES; i++ )
    {
        m_aBubbles[i].Disable();
        vid.sprites[BUBBLE_SPRITEINDEX + i].hide();
    }

    m_bActive = false;
}


void BubbleSpawner::Update( float dt, const Float2 &tilt )
{
    int slotAvailable = -1;
    m_bActive = false;

    for( unsigned int i = 0; i < MAX_BUBBLES; i++ )
    {
        if( m_aBubbles[i].isAlive() )
        {
            m_aBubbles[i].Update( dt, tilt );
            m_bActive = true;
        }
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
            m_bActive = true;
        }
    }
}



void BubbleSpawner::Draw( VideoBuffer &vid, CubeWrapper *pWrapper )
{
    for( unsigned int i = 0; i < MAX_BUBBLES; i++ )
    {
        unsigned int spriteindex = BUBBLE_SPRITEINDEX + i;
        if( m_aBubbles[i].isAlive() )
            m_aBubbles[i].Draw( vid, spriteindex, pWrapper );
        else
            vid.sprites[spriteindex].hide();
    }
}


const float Bubble::BUBBLE_LIFETIME = 1.5f;
const float Bubble::TILT_VEL = 128.0f;
const float Bubble::BEHIND_CHROMITS_THRESHOLD = 0.9f;
//at this depth, bubbles will move away from chromits
const float Bubble::CHROMITS_COLLISION_DEPTH = BEHIND_CHROMITS_THRESHOLD * 0.66666f;
const float Bubble::CHROMIT_OBSCURE_DIST = 15.0f;
const float Bubble::CHROMIT_OBSCURE_DIST_2 = CHROMIT_OBSCURE_DIST * CHROMIT_OBSCURE_DIST;

Bubble::Bubble() : m_fTimeAlive( -1.0f ), m_pTex( &bubbles1 )
{
}

void Bubble::Spawn()
{
    m_pos.set( 64.0f + Game::random.uniform( -SPAWN_EXTENT, SPAWN_EXTENT ), 64.0f + Game::random.uniform( -SPAWN_EXTENT, SPAWN_EXTENT ) );
    m_fTimeAlive = 0.0f;

    if( Game::random.random() < 0.5f )
        m_pTex = &bubbles1;
    else
        m_pTex = &bubbles2;
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

void Bubble::Draw( VideoBuffer &vid, int index, CubeWrapper *pWrapper )
{
    unsigned int frame = m_fTimeAlive / BUBBLE_LIFETIME * m_pTex->numFrames();
    bool visible = true;

    if( frame >= m_pTex->numFrames() )
        frame = m_pTex->numFrames() - 1;

    //sometimes bubbles are obscured by chromits
    if( m_fTimeAlive / BUBBLE_LIFETIME < BEHIND_CHROMITS_THRESHOLD )
    {
        //find our center
        Float2 center = m_pos + m_pTex->pixelExtent();
        Int2 gridslot = { center.x / 32, center.y / 32 };

        GridSlot *pSlot = pWrapper->GetSlot( gridslot.y, gridslot.x );

        if( pSlot && !pSlot->isEmpty() )
        {
            //if our center is inside a certain radius, we are hidden
            Float2 slotcenter = { gridslot.x * 32 + 16, gridslot.y * 32 + 16 };
            Float2 diff = center - slotcenter;

            if( diff.len2() < CHROMIT_OBSCURE_DIST_2 )
            {
                visible = false;

                //move away from chromit
                if( m_fTimeAlive / BUBBLE_LIFETIME > CHROMITS_COLLISION_DEPTH )
                {
                    visible = true;
                    diff = diff.normalize();
                    //m_pos += ( diff * CHROMIT_OBSCURE_DIST );
                    m_pos += diff;
                    //DEBUG_LOG(( "moving bubble to %0.2f, %0.2f\n", m_pos.x, m_pos.y ));

                    //bump the chromit
                    pSlot->Bump( diff );
                }
            }
        }
    }

    if( visible )
    {
        vid.sprites[index].setImage(*m_pTex, frame);
        vid.sprites[index].move(m_pos);
    }
    else
        vid.sprites[index].hide();
}
