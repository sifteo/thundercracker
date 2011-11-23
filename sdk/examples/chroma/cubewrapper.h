/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBEWRAPPER_H
#define _CUBEWRAPPER_H

#include <sifteo.h>
#include "GridSlot.h"
#include "banner.h"
//#include "BG1Helper.h"

using namespace Sifteo;

//wrapper for a cube.  Contains the cube instance and video buffers, along with associated game information
class CubeWrapper
{
public:
	static const int NUM_ROWS = 4;
	static const int NUM_COLS = 4;
	static const int STARTING_SHAKES = 3;
    static const float SHAKE_FILL_DELAY;
	static const int DEFAULT_COHESION = 3;
    static const float SPRING_K_CONSTANT;
    static const float SPRING_DAMPENING_CONSTANT;

	typedef enum
	{
		STATE_PLAYING,
		STATE_EMPTY,
		STATE_NOSHAKES,
	} CubeState;

	CubeWrapper();

	void Init( AssetGroup &assets );
	void Reset();
	//draw loading progress.  return true if done
	bool DrawProgress( AssetGroup &assets );
	void Draw();
    void Update(float t, float dt);
	void vidInit();
	void Tilt( int dir );
	void Shake( bool bShaking );

    Banner &getBanner() { return m_banner; }

	bool isFull();
	bool isEmpty();
	void checkEmpty();

	void checkRefill();
	void Refill( bool bAddLevel = false );

	void testMatches();
	void FillSlotArray( GridSlot **gems, int side, bool clockwise );

	int GetSideNeighboredOn( _SYSCubeID id, Cube &cube );

	//get the number of dots that are marked or exploding
	unsigned int getNumMarked() const;

	GridSlot *GetSlot( int row, int col );
	Cube &GetCube() { return m_cube; }

	//fill in which colors we're using
	void fillColorMap( bool *pMap ) const;
	//do we have the given color anywhere?
	bool hasColor( unsigned int color ) const;

	//do we have stranded fixed dots?
	bool hasStrandedFixedDots() const;
	bool allFixedDotsAreStrandedSide() const;
	unsigned int getNumDots() const;
	unsigned int getNumCornerDots() const;
	//returns if we have one and only one fixed dot (and zero floating dots)
	//fills in the position of that dot
	bool getFixedDot( Vec2 &pos ) const;

	bool isDead() const { return m_state == STATE_NOSHAKES; }
	CubeState getState() const { return m_state; }

private:
	//try moving a gem from row1/col1 to row2/col2
	//return if successful
	bool TryMove( int row1, int col1, int row2, int col2 );

	Cube m_cube;
	VidMode_BG0 m_vid;
	VidMode_BG0_ROM m_rom;
	BG1Helper m_bg1helper;

	CubeState m_state;
	GridSlot m_grid[NUM_ROWS][NUM_COLS];
	Banner m_banner;

	//neighbor info
	int m_neighbors[NUM_SIDES];
	unsigned int m_ShakesRemaining;
	//what time did we start shaking?
	float m_fShakeTime;

    //render based on current fluid level
    //use (-128, 128) range since that matches accelerometer
    Float2 m_curFluidDir;
    Float2 m_curFluidVel;
};

#endif
