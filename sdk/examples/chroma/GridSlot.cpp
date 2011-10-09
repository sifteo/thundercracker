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

GridSlot::GridSlot() : m_state( STATE_LIVING )
{
	static unsigned int seed;
	m_color = rand_r(&seed)%NUM_COLORS;
	m_value = 0;
}


const AssetImage &GridSlot::GetTexture() const
{
	PRINT("color is %d\n", m_color );
	return *TEXTURES[ m_color ];
}
