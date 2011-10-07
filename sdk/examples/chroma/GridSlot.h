/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _GRIDSLOT_H
#define _GRIDSLOT_H

#include <sifteo.h>

using namespace Sifteo;

//space for a gem
class GridSlot
{
public:
	static const int NUM_COLORS = 4;
	static const AssetImage *TEXTURES[ NUM_COLORS ];

	typedef enum 
	{
		STATE_LIVING,
		STATE_BEINGDESTROYED,
		STATE_SHOWINGSCORE,
		STATE_GONE,
	} SLOT_STATE;

	GridSlot();

	const AssetImage &GetTexture() const;
	//draw self on given vid at given vec
	void Draw( VidMode_BG0 &vid, const Vec2 &vec );
	bool isAlive() const { return m_state == STATE_LIVING; }
	bool isEmpty() const { return m_state == STATE_GONE; }
	void setEmpty() { m_state = STATE_GONE; }
private:
	
	SLOT_STATE m_state;
	unsigned int m_color;
	unsigned int m_value;
};


#endif