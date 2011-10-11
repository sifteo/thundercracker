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
	typedef enum
	{
		STATE_SPLASH,
		STATE_MENU,
		STATE_PLAYING,
		STATE_GFXDEMO,
	} GameState;

	static Game &Inst();
	
	Game();

	static const int NUM_CUBES = 2;

	CubeWrapper cubes[NUM_CUBES]; 

	void Init();
	void Update();
	
	//flag self to test matches
	void setTestMatchFlag() { m_bTestMatches = true; }

	unsigned int getIncrementScore() { return ++m_iGemScore; }

	//get random value from 0 to max
	static unsigned int Rand( unsigned int max );

private:
	void TestMatches();
	bool IsAllQuiet();
	bool m_bTestMatches;
	unsigned int m_iGemScore;
	GameState m_state;
	float m_splashTime;
	int m_quietCount;
};

#endif