/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "GridSlot.h"
#include "assets.gen.h"
#include <stdlib.h>
#include "utils.h"

static unsigned int color = 0;

const AssetImage *GridSlot::TEXTURES[ GridSlot::NUM_COLORS ] = 
{
	&Gem0,
	&Gem1,
	&Gem2,
	&Gem3,
};

GridSlot::GridSlot() : 
	//m_state( STATE_LIVING ),
	m_eventTime( 0.0f ),
{
	//TEST randomly make some empty ones
	if( rand()%100 > 50 )
		m_state = STATE_LIVING;
	else
		m_state = STATE_GONE;
	//static unsigned int seed;
	//m_color = rand_r(&seed)%NUM_COLORS;
	//non-random for now
	m_color = color++%NUM_COLORS;
	m_value = 0;
}


const AssetImage &GridSlot::GetTexture() const
{
	PRINT("color is %d\n", m_color );
	return *TEXTURES[ m_color ];
}

//draw self on given vid at given vec
void GridSlot::Draw( VidMode_BG0 &vid, const Vec2 &vec )
{
	switch( m_state )
	{
		case STATE_LIVING:
		{
			const AssetImage &tex = GetTexture();
			vid.BG0_drawAsset(vec, tex, 0);
			break;
		}
		case STATE_MARKED:
		{
			const AssetImage &tex = GetTexture();
			vid.BG0_drawAsset(vec, tex, 1);
			break;
		}
		case STATE_EXPLODING:
		{
			const AssetImage &tex = GetTexture();
			vid.BG0_drawAsset(vec, tex, 4);
			break;
		}
		case STATE_SHOWINGSCORE:
		{
			vid.BG0_text(vec, Font, "0");
			break;
		}
		case STATE_GONE:
		{
			vid.BG0_drawAsset(vec, GemEmpty, 0);
			break;
		}
		default:
			break;
	}
	
}


void GridSlot::Update(float t)
{
	switch( m_state )
	{
		case STATE_MARKED:
		{
			if( t - m_eventTime > MARK_SPREAD_DELAY )
                spread_mark();
            if( t - m_eventTime > MARK_BREAK_DELAY )
                explode();
			break;
		}
		case STATE_EXPLODING:
		{
			if( t - m_eventTime > MARK_EXPLODE_DELAY )
                die();
			break;
		}
		default:
			break;
	}
}


void GridSlot::mark()
{
	m_state = STATE_MARKED;
	m_eventTime = System::clock();
}


void GridSlot::spread_mark()
{
}

void GridSlot::explode()
{
	m_state = STATE_EXPLODING;
	m_eventTime = System::clock();
}

void GridSlot::die()
{
	m_state = STATE_SHOWINGSCORE;
	m_eventTime = System::clock();
}
