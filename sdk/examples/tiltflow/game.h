/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _GAME_H
#define _GAME_H

#include <sifteo.h>
#include "cubewrapper.h"
#include "TiltFlowMenu.h"

using namespace Sifteo;

//singleton class
class Game
{
public:    
	static Game &Inst();
	
	Game();

	static const int NUM_CUBES = 2;
    static const int NUM_MENU_ITEMS = 3;

	CubeWrapper cubes[NUM_CUBES]; 

	void Init();
	void Update();
	void Reset();
	

private:
    float m_fLastTime;
    TiltFlowMenu m_Menu;
};

#endif
