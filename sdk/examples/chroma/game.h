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
		STARTING_STATE = STATE_PLAYING,
	} GameState;

	typedef enum
	{
		MODE_FLIPS,
		MODE_TIMED,
		MODE_PUZZLE,
	} GameMode;

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

	inline GameState getState() const { return m_state; }
	inline GameMode getMode() const { return m_mode; }

	inline unsigned int getScore() const { return m_iScore; }

private:
	void TestMatches();
	bool IsAllQuiet();
	bool m_bTestMatches;
	unsigned int m_iGemScore;
	unsigned int m_iScore;
	GameState m_state;
	GameMode m_mode;
	float m_splashTime;
};

#endif