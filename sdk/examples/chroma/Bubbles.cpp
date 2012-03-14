/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * handles bubble sprites
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "Bubbles.h"
#include "game.h"
#include "assets.gen.h"

//min/max for short period
const float BubbleSpawner::MIN_SHORT_SPAWN = 0.1f;
const float BubbleSpawner::MAX_SHORT_SPAWN = 0.6f;
//min/max for long period
const float BubbleSpawner::MIN_LONG_SPAWN = 8.0f;
const float BubbleSpawner::MAX_LONG_SPAWN = 12.0f;
//chance that we do a short period
const float BubbleSpawner::SHORT_PERIOD_CHANCE = 0.75f;


BubbleSpawner::BubbleSpawner()
{
    Reset();
}


void BubbleSpawner::Reset()
{
    m_fTimeTillSpawn = Game::random.uniform( MAX_SHORT_SPAWN, MIN_LONG_SPAWN );

    for( unsigned int i = 0; i < MAX_BUBBLES; i++ )
    {
        m_aBubbles[i].Disable();
    }
}


void BubbleSpawner::Update( float dt )
{
    int slotAvailable = -1;

    for( unsigned int i = 0; i < MAX_BUBBLES; i++ )
    {
        if( m_aBubbles[i].isAlive() )
            m_aBubbles[i].Update( dt );
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



void BubbleSpawner::Draw( VidMode_BG0_SPR_BG1 &vid )
{
    for( unsigned int i = 0; i < MAX_BUBBLES; i++ )
    {
        unsigned int spriteindex = BUBBLE_SPRITEINDEX + i;
        if( m_aBubbles[i].isAlive() )
            m_aBubbles[i].Draw( vid, spriteindex );
        else
            vid.resizeSprite(spriteindex, 0, 0);
    }
}


const float Bubble::BUBBLE_LIFETIME = 2.5f;

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

void Bubble::Update( float dt )
{
    m_fTimeAlive += dt;

    if( m_fTimeAlive >= BUBBLE_LIFETIME )
        Disable();
}

void Bubble::Draw( VidMode_BG0_SPR_BG1 &vid, int index )
{
    unsigned int frame = m_fTimeAlive / BUBBLE_LIFETIME * bubbles.frames;

    if( frame >= bubbles.frames )
        frame = bubbles.frames - 1;
    vid.resizeSprite(index, bubbles.width*8, bubbles.height*8);
    vid.setSpriteImage(index, bubbles, frame);
    vid.moveSprite(index, m_pos.x, m_pos.y);
}
