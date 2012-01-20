/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _GAME_H
#define _GAME_H

#include <sifteo.h>
#include "Level.h"
#include "cubewrapper.h"
#include "TimeKeeper.h"
#include "config.h"

using namespace Sifteo;

//singleton class
class Game
{
public:    
	typedef enum
	{
		STATE_SPLASH,
#if SPLASH_ON
        STARTING_STATE = STATE_SPLASH,
#endif
		STATE_MENU,
        STATE_INTRO,
#if !SPLASH_ON
        STARTING_STATE = STATE_INTRO,
#endif
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

    //static const int NUM_CUBES = 3;
    static const unsigned int NUM_HIGH_SCORES = 5;
    static const int STARTING_SHAKES = 3;
    static const unsigned int NUM_SFX_CHANNELS = 3;
    static const int NUM_SLOSH_SOUNDS = 2;
    static const unsigned int INT_MAX = 0x7fff;
    static const float SLOSH_THRESHOLD;

    //number of dots needed for certain thresholds
    enum
    {
        DOT_THRESHOLD1 = 2,
        DOT_THRESHOLD2 = 4,
        DOT_THRESHOLD3 = 9,
        DOT_THRESHOLD4 = 14,
        DOT_THRESHOLD5 = 15,
    };

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
    inline const Level &getLevel() const { return Level::GetLevel( m_iLevel ); }
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

    void playSound( _SYSAudioModule &sound );
    //play random slosh sound
    void playSlosh();

    inline void forcePaintSync() { m_bForcePaintSync = true; }

    inline unsigned int getShakesLeft() const { return m_ShakesRemaining; }
    inline void useShake() { m_ShakesRemaining--; }

    //destroy all dots of the given color
    void BlowAll( unsigned int color );

private:
	void TestMatches();
    bool DoesHyperDotExist();
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
    float m_fLastSloshTime;

#if SFX_ON
    AudioChannel m_SFXChannels[NUM_SFX_CHANNELS];
#endif
    //which channel to use
    unsigned m_curChannel;
#if MUSIC_ON
    AudioChannel m_musicChannel;
#endif
    //use to avoid playing the same sound multiple times in one frame
    const _SYSAudioModule *m_pSoundThisFrame;

    static unsigned int s_HighScores[ NUM_HIGH_SCORES ];
    unsigned int m_ShakesRemaining;

    //force a 1 frame paint sync before/after drawing
    bool m_bForcePaintSync;
    //keeps track of whether a hyperdot was used this chain
    //bool m_bHyperDotMatched;
};

#endif
