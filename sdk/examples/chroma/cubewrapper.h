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
	static const int NUM_ROWS = 4;
	static const int NUM_COLS = 4;

	CubeWrapper( _SYSCubeID id );

	void Init( AssetGroup &assets );
	//draw loading progress.  return true if done
	bool DrawProgress( AssetGroup &assets );
	void Draw();
	void vidInit();
private:
	Cube m_cube;
	VidMode_BG0 m_vid;
	VidMode_BG0_ROM m_rom;
	GridSlot m_grid[NUM_ROWS][NUM_COLS];
};

#endif