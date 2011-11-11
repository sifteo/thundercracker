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
		STATE_POSTGAME,
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
	void Reset();
	
	//flag self to test matches
	void setTestMatchFlag() { m_bTestMatches = true; }

	unsigned int getIncrementScore() { m_iDotScoreSum += ++m_iDotScore; return m_iDotScore; }

	//get random value from 0 to max
	static unsigned int Rand( unsigned int max );

	inline GameState getState() const { return m_state; }
	inline GameMode getMode() const { return m_mode; }

	inline unsigned int getScore() const { return m_iScore; }
	inline unsigned int getLevel() const { return m_iLevel; }
	inline void addLevel() { m_iLevel++; }

	void CheckChain( CubeWrapper *pWrapper );
	void checkGameOver();
	bool NoMatches();
	unsigned int numColors() const;
	bool no_match_color_imbalance() const;
	bool no_match_stranded_interior() const;
	bool no_match_stranded_side() const;
	bool no_match_mismatch_side() const;

private:
	void TestMatches();
	bool m_bTestMatches;
	//how much our current dot is worth
	unsigned int m_iDotScore;
	//running total
	unsigned int m_iDotScoreSum;
	unsigned int m_iScore;
	unsigned int m_iDotsCleared;
	//for progression in flips mode
	unsigned int m_iLevel;
	GameState m_state;
	GameMode m_mode;
	float m_splashTime;
};

#endif