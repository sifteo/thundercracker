/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _GAME_H
#define _GAME_H

#include <sifteo.h>
#include "cubewrapper.h"
#include "TimeKeeper.h"

using namespace Sifteo;

//singleton class
class Game
{
public:    
	typedef enum
	{
		STATE_SPLASH,
		//STARTING_STATE = STATE_SPLASH,
		STATE_MENU,
        STATE_INTRO,
        STARTING_STATE = STATE_INTRO,
		STATE_PLAYING,		
        STATE_DYING,
		STATE_POSTGAME,
	} GameState;

	typedef enum
	{
		MODE_SHAKES,
		MODE_TIMED,
		MODE_PUZZLE,
	} GameMode;

	static Game &Inst();
	
	Game();

	static const int NUM_CUBES = 2;
    static const unsigned int NUM_HIGH_SCORES = 5;
    static const unsigned int INT_MAX = 0x7fff;

	CubeWrapper cubes[NUM_CUBES]; 

	void Init();
	void Update();
	void Reset();
	
	//flag self to test matches
	void setTestMatchFlag() { m_bTestMatches = true; }

	unsigned int getIncrementScore() { m_iDotScoreSum += ++m_iDotScore; return m_iDotScore; }

	//get random value from 0 to max
	static unsigned int Rand( unsigned int max );
    //get random float value from 0 to 1.0
    static float UnitRand();
    //get random value from min to max
    static float RandomRange( float min, float max );

	inline GameState getState() const { return m_state; }
    inline void setState( GameState state ) { m_state = state; }
	inline GameMode getMode() const { return m_mode; }

	inline unsigned int getScore() const { return m_iScore; }
	inline unsigned int getLevel() const { return m_iLevel; }
	inline void addLevel() { m_iLevel++; }

	TimeKeeper &getTimer() { return m_timer; }
    unsigned int getHighScore( unsigned int index ) const;
    void enterScore();

	void CheckChain( CubeWrapper *pWrapper );
	void checkGameOver();
	bool NoMatches();
	unsigned int numColors() const;
	bool no_match_color_imbalance() const;
	bool no_match_stranded_interior() const;
	bool no_match_stranded_side() const;
	bool no_match_mismatch_side() const;

    void playSound( const _SYSAudioModule &sound );

private:
	void TestMatches();
	bool m_bTestMatches;
	//how much our current dot is worth
	unsigned int m_iDotScore;
	//running total
	unsigned int m_iDotScoreSum;
	unsigned int m_iScore;
	unsigned int m_iDotsCleared;
	//for progression in shakes mode
	unsigned int m_iLevel;
	GameState m_state;
	GameMode m_mode;
	float m_splashTime;
	TimeKeeper m_timer;
    float m_fLastTime;

    AudioChannel m_SFXChannel;
    AudioChannel m_musicChannel;

    static unsigned int s_HighScores[ NUM_HIGH_SCORES ];
};

#endif
