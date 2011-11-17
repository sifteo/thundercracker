/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _GRIDSLOT_H
#define _GRIDSLOT_H

#include <sifteo.h>

using namespace Sifteo;
class CubeWrapper;

//space for a gem
class GridSlot
{
public:
	static const unsigned int NUM_COLORS = 8;
	static const AssetImage *TEXTURES[ NUM_COLORS ];
	static const unsigned int NUM_ROLLING_COLORS = 2;
	static const AssetImage *ROLLING_TEXTURES[ NUM_ROLLING_COLORS ];
	static const AssetImage *ROLLING_ANIM[ NUM_ROLLING_COLORS ];

	static const float MARK_SPREAD_DELAY = 0.33f;
	static const float MARK_BREAK_DELAY = 0.67f;
	static const float MARK_EXPLODE_DELAY = 0.16f;
	static const float SCORE_FADE_DELAY = 2.0;

	typedef enum 
	{
		STATE_LIVING,
		STATE_PENDINGMOVE,
		STATE_MOVING,
		STATE_FINISHINGMOVE,
		STATE_MARKED,
		STATE_EXPLODING,
		STATE_SHOWINGSCORE,
		STATE_GONE,
	} SLOT_STATE;

	GridSlot();

	void Init( CubeWrapper *pWrapper, unsigned int row, unsigned int col ); 
	const AssetImage &GetTexture() const;
	//draw self on given vid at given vec
	void Draw( VidMode_BG0 &vid, unsigned int tiltMask );
	void Update(float t);
	bool isAlive() const { return m_state == STATE_LIVING; }
	bool isEmpty() const { return m_state == STATE_GONE; }
	bool isMarked() const { return ( m_state == STATE_MARKED || m_state == STATE_EXPLODING ); }
	bool isTiltable() const { return ( m_state == STATE_LIVING || m_state == STATE_PENDINGMOVE ); }
	void setEmpty() { m_state = STATE_GONE; }
	unsigned int getColor() const { return m_color; }
	void FillColor(unsigned int color);

	void mark();
	void spread_mark();
	void explode();
	void die();

	bool IsFixed() const { return m_bFixed; }
	void MakeFixed() { m_bFixed = true; }

	//copy color and some other attributes from target.  Used when tilting
	void TiltFrom(GridSlot &src);
	//if we have a move pending, start it
	void startPendingMove();
private:
	void markNeighbor( int row, int col );
	//given a tiltmask, calculate the roll frame we should be in
	unsigned int CalculateRollFrame( unsigned int tiltMask ) const;

	SLOT_STATE m_state;
	unsigned int m_color;
	float m_eventTime;
	CubeWrapper *m_pWrapper;
	unsigned int m_row;
	unsigned int m_col;

	//current position in 16x16 grid for use when moving
	Vec2 m_curMovePos;

	unsigned int m_score;
	//fixed dot
	bool		 m_bFixed;

	unsigned int m_animFrame;
};


#endif