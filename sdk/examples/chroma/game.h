/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _GAME_H
#define _GAME_H

#include <sifteo.h>
#include "cubewrapper.h"

using namespace Sifteo;

//singleton class
class Game
{
public:
	//compiler seems to barf on this, so I'll skip it for now
	static Game &Inst();
	
	Game();

	/*static const int NUM_CUBES = 2;

static CubeWrapper cubes[NUM_CUBES] = 
{
	CubeWrapper((_SYSCubeID)0),
	CubeWrapper((_SYSCubeID)1),
};*/
	static const int NUM_CUBES = 1;

	CubeWrapper cubes[NUM_CUBES]; 

	void Init();
	void Update();
	
private:
	
};

#endif