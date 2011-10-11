/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBEWRAPPER_H
#define _CUBEWRAPPER_H

#include <sifteo.h>
#include "GridSlot.h"

using namespace Sifteo;

//wrapper for a cube.  Contains the cube instance and video buffers, along with associated game information
class CubeWrapper
{
public:
	static const int NUM_SIDES = 4;
	static const int NUM_ROWS = 4;
	static const int NUM_COLS = 4;

	CubeWrapper();

	void Init( AssetGroup &assets );
	//draw loading progress.  return true if done
	bool DrawProgress( AssetGroup &assets );
	void Draw();
	void DrawSplash();
	void Update(float t);
	void vidInit();
	void Tilt( int dir );

	void testMatches();
	void FillSlotArray( GridSlot **gems, int side, bool clockwise );

	//run hello world
	void HelloInit();
	void Hello();
	void HelloTilt(_SYSAccelState &state);
	//run Beth's gfxdemo
	void Demo();

	int GetSideNeighboredOn( _SYSCubeID id, Cube &cube );

	//if all gems are living or gone, nothing is happening
	bool IsQuiet() const;

	GridSlot *GetSlot( int row, int col );
private:
	//try moving a gem from row1/col1 to row2/col2
	//return if successful
	bool TryMove( int row1, int col1, int row2, int col2 );

	Cube m_cube;
	VidMode_BG0 m_vid;
	VidMode_BG0_ROM m_rom;
	GridSlot m_grid[NUM_ROWS][NUM_COLS];
	unsigned int m_frame;

	//neighbor info
	//right now doesn't know what cubes are on what sides
	bool m_neighbors[NUM_SIDES];
};

#endif