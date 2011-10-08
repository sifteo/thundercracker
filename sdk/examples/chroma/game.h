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

	//static const int NUM_CUBES = 2;

	static const int NUM_CUBES = 1;

	CubeWrapper cubes[NUM_CUBES]; 

	void Init();
	void Update();
	
	//flag self to test matches
	void setTestMatchFlag() { m_bTestMatches = true; }

private:
	void TestMatches();
	bool m_bTestMatches;
};

#endif