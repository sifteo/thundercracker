/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "GridSlot.h"
#include "assets.gen.h"
#include <stdlib.h>
#include "utils.h"

const AssetImage *GridSlot::TEXTURES[ GridSlot::NUM_COLORS ] = 
{
	&Gem0,
	&Gem1,
	&Gem2,
	&Gem3,
};

GridSlot::GridSlot() //: m_state( STATE_LIVING )
{
	//TEST randomly make some empty ones
	if( rand()%100 > 50 )
		m_state = STATE_LIVING;
	else
		m_state = STATE_GONE;
	m_color = rand()%NUM_COLORS;
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
		case STATE_GONE:
		{
			vid.BG0_drawAsset(vec, GemEmpty, 0);
			break;
		}
		default:
			break;
	}
	
}