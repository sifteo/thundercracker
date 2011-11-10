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

using namespace Sifteo;

//wrapper for a cube.  Contains the cube instance and video buffers, along with associated game information
class CubeWrapper
{
public:
	static const int NUM_ROWS = 4;
	static const int NUM_COLS = 4;
	static const int STARTING_FLIPS = 3;
	static const float SHAKE_FILL_DELAY = 1.0f;
	static const int DEFAULT_COHESION = 3;

	typedef enum
	{
		STATE_PLAYING,
		STATE_EMPTY,
		STATE_NOFLIPS,
	} CubeState;

	CubeWrapper();

	void Init( AssetGroup &assets );
	//draw loading progress.  return true if done
	bool DrawProgress( AssetGroup &assets );
	void Draw();
	void DrawSplash();
	void Update(float t);
	void vidInit();
	void Tilt( int dir );
	void Shake( bool bShaking );

	Banner &getBanner() { return m_banner; }

	bool isFull();
	bool isEmpty();

	void checkRefill();
	void Refill( bool bAddLevel = false );

	void testMatches();
	void FillSlotArray( GridSlot **gems, int side, bool clockwise );

	int GetSideNeighboredOn( _SYSCubeID id, Cube &cube );

	//get the number of dots that are marked or exploding
	unsigned int getNumMarked() const;

	GridSlot *GetSlot( int row, int col );
	Cube &GetCube() { return m_cube; }
private:
	//try moving a gem from row1/col1 to row2/col2
	//return if successful
	bool TryMove( int row1, int col1, int row2, int col2 );

	Cube m_cube;
	VidMode_BG0 m_vid;
	VidMode_BG0_ROM m_rom;

	CubeState m_state;
	GridSlot m_grid[NUM_ROWS][NUM_COLS];
	Banner m_banner;

	//neighbor info
	int m_neighbors[NUM_SIDES];
	unsigned int m_flipsRemaining;
	//what time did we start shaking?
	float m_fShakeTime;
};

#endif