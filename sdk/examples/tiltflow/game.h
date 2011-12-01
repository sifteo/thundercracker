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
	static Game &Inst();
	
	Game();

	static const int NUM_CUBES = 2;
    static const unsigned int NUM_HIGH_SCORES = 5;
    static const unsigned int INT_MAX = 0x7fff;

	CubeWrapper cubes[NUM_CUBES]; 

	void Init();
	void Update();
	void Reset();
	

private:
    float m_fLastTime;
};

#endif
