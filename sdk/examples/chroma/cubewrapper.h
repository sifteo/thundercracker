/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBEWRAPPER_H
#define _CUBEWRAPPER_H

#include <sifteo.h>
#include "banner.h"
#include "Bubbles.h"
#include "FloatingScore.h"
#include "GameOver.h"
#include "Glimmer.h"
#include "GridSlot.h"
#include "Intro.h"
#include "RockExplosion.h"

using namespace Sifteo;

//wrapper for a cube.  Contains the cube instance and video buffers, along with associated game information
class CubeWrapper
{
public:
	static const int NUM_ROWS = 4;
	static const int NUM_COLS = 4;
    static const float SHAKE_FILL_DELAY;
	static const int DEFAULT_COHESION = 3;
    static const float SPRING_K_CONSTANT;
    static const float SPRING_DAMPENING_CONSTANT;
    static const float MOVEMENT_THRESHOLD;
    static const float IDLE_TIME_THRESHOLD;
    static const float IDLE_FINISH_THRESHOLD;
    static const float MIN_GLIMMER_TIME;
    static const float MAX_GLIMMER_TIME;
    static const float TOUCH_TIME_FOR_MENU;

    static const int TEST_TILT_ITERATIONS = 4;
    //anything below this we don't care about
    static const float TILT_SOUND_EPSILON;
    static const int PTS_PER_EMPTIED_CUBE = 100;
    //static const float SHOW_BONUS_TIME;
    static const int HIGH_SCORE_OFFSET = 4;

	typedef enum
	{
		STATE_PLAYING,
		STATE_EMPTY,
        STATE_REFILL,
        //STATE_CUBEBONUS,
	} CubeState;

	CubeWrapper();

	void Init( AssetGroup &assets );
	void Reset();
	//draw loading progress.  return true if done
	bool DrawProgress( AssetGroup &assets );
	void Draw();
    void Update(float t, float dt);
	void vidInit();
	void Tilt( int dir );
    static bool FakeTilt( int dir, GridSlot grid[][NUM_COLS] );
	void Shake( bool bShaking );
    void Touch();

    Banner &getBanner() { return m_banner; }

    bool isFull() const;
    bool isEmpty() const;
	void checkEmpty();

	void checkRefill();
    void Refill();

	void testMatches();
	void FillSlotArray( GridSlot **gems, int side, bool clockwise );

	int GetSideNeighboredOn( _SYSCubeID id, Cube &cube );

	//get the number of dots that are marked or exploding
	unsigned int getNumMarked() const;

	GridSlot *GetSlot( int row, int col );
	Cube &GetCube() { return m_cube; }

	//fill in which colors we're using
	void fillColorMap( bool *pMap ) const;
	//do we have the given color anywhere?
	bool hasColor( unsigned int color ) const;

	//do we have stranded fixed dots?
	bool hasStrandedFixedDots() const;
	bool allFixedDotsAreStrandedSide() const;
	unsigned int getNumDots() const;
	unsigned int getNumCornerDots() const;
	//returns if we have one and only one fixed dot (and zero floating dots)
	//fills in the position of that dot
	bool getFixedDot( Vec2 &pos ) const;
    bool hasNonStrandedDot() const;

	CubeState getState() const { return m_state; }
    void setState( CubeState state );
    //bool IsIdle() const;
    inline int getLastTiltDir() const { return m_lastTiltDir; }
    inline Float2 getTiltDir() const { return m_curFluidDir; }
    inline BG1Helper &getBG1Helper() { return m_bg1helper; }

    //if we need to, flush bg1
    void FlushBG1();

    //queue a location to be cleared by gemEmpty.
    //This exists because we need to do all our clears first, and then do our draws
    void QueueClear( Vec2 &pos );
    void SpawnSpecial( unsigned int color );
    bool SpawnMultiplier( unsigned int mult );
    //destroy all dots of the given color
    void BlowAll( unsigned int color );
    bool HasHyperDot() const;

    //pretend to tilt this cube in a series of tilts, and update whether we see the given color on corners or side patterns 1 or 2
    void UpdateColorPositions( unsigned int color, bool &bCorners, bool &side1, bool &side2 ) const;

    //add one piece
    void RespawnOnePiece();
    //search for a multiplier dot and increase it
    void UpMultiplier();
    void ClearSprite( unsigned int id );
    void TurnOffSprites();
    inline void resetIntro() { m_intro.Reset(); }
    inline void setDirty() { m_dirty = true; }

    void StopGlimmer();
    void SpawnRockExplosion( const Vec2 &pos, unsigned int health );
    //each cube can have one floating score at a time
    void SpawnScore( unsigned int score, const Vec2 &slotpos );

private:
	//try moving a gem from row1/col1 to row2/col2
	//return if successful
	bool TryMove( int row1, int col1, int row2, int col2 );
    static bool FakeTryMove( int row1, int col1, int row2, int col2, GridSlot grid[][NUM_COLS] );

    //check different parts of the given grid for the given color
    static void TestGridForColor( const GridSlot grid[][NUM_COLS], unsigned int color, bool &bCorners, bool &side1, bool &side2 );
    //recursive function to tilt and test grid
    static void TiltAndTestGrid( GridSlot grid[][NUM_COLS], unsigned int color, bool &bCorners, bool &side1, bool &side2, int iterations );

    bool HasFloatingDots() const;
    void fillPuzzleCube();
    //draw a message box with centered text
    //bDrawBox - draw the box or not
    //in_yOffset - optional y offset for text
    void DrawMessageBoxWithText( const char *pTxt, bool bDrawBox = true, int in_yOffset = 0 );
    void DrawGrid();

	Cube m_cube;
    VidMode_BG0_SPR_BG1 m_vid;
	VidMode_BG0_ROM m_rom;
	BG1Helper m_bg1helper;

	CubeState m_state;
	GridSlot m_grid[NUM_ROWS][NUM_COLS];
	Banner m_banner;

	//neighbor info
	int m_neighbors[NUM_SIDES];
	//what time did we start shaking?
	float m_fShakeTime;
    //how long have we been touching the cube?
    float m_fTouchTime;

    //render based on current fluid level
    //use (-128, 128) range since that matches accelerometer
    Float2 m_curFluidDir;
    Float2 m_curFluidVel;
    //how long have we been not tilting
    float m_idleTimer;

    Intro m_intro;
    GameOver m_gameover;
    Glimmer m_glimmer;

    float m_timeTillGlimmer;

    float m_stateTime;
    int m_lastTiltDir;

    //array of queued clears.
    //clears get queued up in update, then they get drawn before any draws and cleared out
    Vec2 m_queuedClears[NUM_ROWS * NUM_COLS];
    int m_numQueuedClears;

    //do we need to do a bg1 flush?
    bool m_queuedFlush;
    //TODO, need to start using this for other screens
    bool m_dirty;

    //allow up to 4 rock explosions simultaneously
    RockExplosion m_aExplosions[ RockExplosion::MAX_ROCK_EXPLOSIONS ];
    BubbleSpawner m_bubbles;
    FloatingScore m_floatscore;
};

#endif
