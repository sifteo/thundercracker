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
#include "SaveLoad.h"

using namespace Sifteo;
struct PuzzleCubeData;
struct Puzzle;

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
        STATE_MAINMENU,
#if !SPLASH_ON
        STARTING_STATE = STATE_MAINMENU,
#endif
        STATE_INTRO,
		STATE_PLAYING,		
		STATE_POSTGAME,
        STATE_GOODJOB,
        STATE_FAILPUZZLE,
        STATE_NEXTPUZZLE,
        STATE_GAMEMENU,
        STATE_GAMEOVERBANNER,

        //more menus
        STATE_PUZZLEMENU,
        STATE_CHAPTERSELECTMENU,
        STATE_PUZZLESELECTMENU,
	} GameState;

	typedef enum
	{
        MODE_SURVIVAL,
        MODE_BLITZ,
		MODE_PUZZLE,
        MODE_CNT
	} GameMode;

	static Game &Inst();
	
	Game();

    static const int STARTING_SHAKES = 0;
    static const unsigned int NUM_SFX_CHANNELS = 3;
    static const int NUM_SLOSH_SOUNDS = 2;
    static const unsigned int INT_MAX = 0x7fff;
    static const float SLOSH_THRESHOLD;
    static const int NUM_COLORS_FOR_HYPER = 3;
    //timer constants
    static const float TIME_TO_RESPAWN;
    static const float COMBO_TIME_THRESHOLD;
    static const int MAX_MULTIPLIER = 7;
    static const float LUMES_FACE_TIME;
    //show lumes + "Good job"
    static const float FULLGOODJOB_TIME;


    //number of dots needed for certain thresholds
    enum
    {
        DOT_THRESHOLD1 = 2,
        DOT_THRESHOLD2 = 4,
        DOT_THRESHOLD_TIMED_RAINBALL = 6,
        DOT_THRESHOLD_TIMED_MULT = 9,
        DOT_THRESHOLD3 = 9,
        DOT_THRESHOLD4 = 14,
        DOT_THRESHOLD5 = 15,
    };

    CubeWrapper m_cubes[NUM_CUBES];
    static Random random;

	void Init();
	void Update();
    void Reset( bool bInGame = true );

    CubeWrapper *GetWrapper( Cube *pCube );
    CubeWrapper *GetWrapper( unsigned int index );
    int getWrapperIndex( const CubeWrapper *pWrapper );

	//flag self to test matches
	void setTestMatchFlag() { m_bTestMatches = true; }

	unsigned int getIncrementScore() { m_iDotScoreSum += ++m_iDotScore; return m_iDotScore; }

	inline GameState getState() const { return m_state; }
    inline float getStateTime() const { return m_stateTime; }
    void setState( GameState state );
    //go to state through bubble transition
    void TransitionToState( GameState state );
	inline GameMode getMode() const { return m_mode; }

    unsigned int getScore() const;
    inline void addScore( unsigned int score ) { m_iScore += score; }
    inline const Level &getLevel() const { return Level::GetLevel( m_iLevel ); }
    inline void addLevel() { m_iLevel++; }
    inline unsigned int getDisplayedLevel() const { return m_iLevel; }

	TimeKeeper &getTimer() { return m_timer; }
    unsigned int getHighScore( unsigned int index ) const;
    void enterScore();

    void CheckChain( CubeWrapper *pWrapper, const Int2 &slotPos );
	void checkGameOver();
	bool NoMatches();
	unsigned int numColors() const;
	bool no_match_color_imbalance() const;
	bool no_match_stranded_interior() const;
	bool no_match_stranded_side() const;
	bool no_match_mismatch_side() const;
    unsigned int NumCubesWithColor( unsigned int color ) const;
    bool IsColorUnmatchable( unsigned int color ) const;
    bool AreAllColorsUnmatchable() const;
    bool DoCubesOnlyHaveStrandedDots() const;
    bool OnlyOneOtherCorner( const CubeWrapper *pWrapper ) const;

    void playSound( const AssetAudio &sound );
    //play random slosh sound
    void playSlosh();

    inline void forcePaintSync() { m_bForcePaintSync = true; }

    inline unsigned int getShakesLeft() const { return m_ShakesRemaining; }
    inline void useShake() { m_ShakesRemaining--; }

    //destroy all dots of the given color
    void BlowAll( unsigned int color );
    void EndGame();

    inline void Stabilize()
    {
        if( !m_bIsChainHappening )
            m_bStabilized = true;
    }

    bool AreNoCubesEmpty() const;
    unsigned int CountEmptyCubes() const;

    inline void SetUsedColor( unsigned int color ) { m_aColorsUsed[color] = true; }
    void UpCombo();
    inline unsigned int GetComboCount() const { return m_comboCount; }
    void UpMultiplier();
    const Puzzle *GetPuzzle();
    const PuzzleCubeData *GetPuzzleData( unsigned int id );
    inline unsigned int GetPuzzleIndex() const { return m_iLevel + 1; }
    inline void SetChain( bool bValue ) { m_bIsChainHappening = bValue; }
    bool AreMovesLegal() const;

    void ReturnToMainMenu();
    void gotoNextPuzzle( bool bAdvance );

private:
	void TestMatches();
    bool DoesHyperDotExist();
    //add one piece to the game
    void RespawnOnePiece();
    void check_puzzle();
    void HandleMenu();

	bool m_bTestMatches;
	//how much our current dot is worth
	unsigned int m_iDotScore;
	//running total
	unsigned int m_iDotScoreSum;
	unsigned int m_iScore;
	unsigned int m_iDotsCleared;
    //how many colors were involved in this
    bool m_aColorsUsed[ GridSlot::NUM_COLORS ];
	//for progression in shakes mode
    uint8_t m_iLevel;
    //used to track which chapter we're looking at in puzzle menus
    uint8_t m_iChapterViewed;
    SaveData m_savedata;
	GameState m_state;
	GameMode m_mode;
    float m_stateTime;
	TimeKeeper m_timer;
    TimeStep m_timeStep;
    SystemTime m_lastSloshTime;

#if SFX_ON
    AudioChannel m_SFXChannels[NUM_SFX_CHANNELS];
#endif
    //which channel to use
    unsigned m_curChannel;
#if MUSIC_ON
    AudioChannel m_musicChannel;
#endif
    //use to avoid playing the same sound multiple times in one frame
    const AssetAudio *m_pSoundThisFrame;

    unsigned int m_ShakesRemaining;
    //how long until we respawn one piece in timer mode
    float m_fTimeTillRespawn;
    //which cube to respawn to next
    unsigned int m_cubeToRespawn;
    unsigned int m_comboCount;
    float m_fTimeSinceCombo;
    unsigned int m_Multiplier;

    //force a 1 frame paint sync before/after drawing
    bool m_bForcePaintSync;
    //keeps track of whether a hyperdot was used this chain
    //bool m_bHyperDotMatched;
    //set to true every time the state of the game is stabilized to run checks on
    bool m_bStabilized;
    bool m_bIsChainHappening;
};

#endif
