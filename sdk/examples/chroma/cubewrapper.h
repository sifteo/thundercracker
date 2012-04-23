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
    static const int HIGH_SCORE_OFFSET = 4;
    //how long we wait until we autorefill an empty cube in survival mode
    static const float AUTOREFILL_TIME;

	typedef enum
	{
		STATE_PLAYING,
		STATE_EMPTY,
        STATE_REFILL,
        STATE_CNT,
	} CubeState;

    typedef struct
    {
        unsigned int color;
        bool bCorners;
        bool bSide1;
        bool bSide2;

        bool TotallySatisfied() const { return bCorners && bSide1 && bSide2; }
    } GridTestInfo;

	CubeWrapper();

    void Init();
	void Reset();

    //draw callbacks, one for STATE_PLAYING, and one for the rest of the states
    void DrawUI();
    void DrawPlaying() __attribute__ ((noinline));
    void DrawInPlay() __attribute__ ((noinline));
    void DrawEmpty() __attribute__ ((noinline));
    void DrawRefill() __attribute__ ((noinline));

    void Draw() __attribute__ ((noinline));
    void Update(SystemTime t, TimeDelta dt) __attribute__ ((noinline));
	void Tilt( int dir );
    static bool FakeTilt( int dir, GridSlot grid[][NUM_COLS] );
	void Shake( bool bShaking );
    void Touch();

    Banner &getBanner() { return m_banner; }

    bool isFull() const;
    bool isEmpty() const;
	void checkEmpty();

    void checkRefill() __attribute__ ((noinline));
    void Refill();

	void testMatches();
	void FillSlotArray( GridSlot **gems, int side, bool clockwise );

    int GetSideNeighboredOn( _SYSCubeID id, CubeID &cube );

	//get the number of dots that are marked or exploding
	unsigned int getNumMarked() const;

	GridSlot *GetSlot( int row, int col );
    CubeID &GetCube() { return m_cube; }

	//fill in which colors we're using
	void fillColorMap( bool *pMap ) const;
	//do we have the given color anywhere?
    //bAllowWilds will count rain/hyper chromits as a color
    bool hasColor( unsigned int color, bool bAllowWilds ) const;

	//do we have stranded fixed dots?
	bool hasStrandedFixedDots() const;
	bool allFixedDotsAreStrandedSide() const;
	unsigned int getNumDots() const;
	unsigned int getNumCornerDots() const;
	//returns if we have one and only one fixed dot (and zero floating dots)
	//fills in the position of that dot
	bool getFixedDot( Int2 &pos ) const;
    bool hasNonStrandedDot() const;

	CubeState getState() const { return m_state; }
    void setState( CubeState state );
    //bool IsIdle() const;
    inline int getLastTiltDir() const { return m_lastTiltDir; }
    inline Float2 getTiltDir() const { return m_curFluidDir; }

    //if we need to, flush bg1
    void testFlushBG1();

    //queue up a col/row to be cleared (4 tiles)
    //used when a chromit is being moved
    //This exists because we need to do all our clears first, and then do our draws
    //index is a col/row index
    //tile is a tile position
    void QueueClear( uint8_t index, uint8_t tile, bool bRow );
    void SpawnSpecial( unsigned int color );
    bool SpawnMultiplier( unsigned int mult );
    //destroy all dots of the given color
    void BlowAll( unsigned int color );
    bool HasHyperDot() const;

    //pretend to tilt this cube in a series of tilts, and update whether we see the given color on corners or side patterns 1 or 2
    void UpdateColorPositions( GridTestInfo &testInfo ) const __attribute__ ((noinline));

    //add one piece
    void RespawnOnePiece();
    //search for a multiplier dot and increase it
    void UpMultiplier();
    void ClearSprite( unsigned int id );
    void TurnOffSprites();
    inline void resetIntro() { m_intro.Reset(); }
    inline void setDirty() { m_dirty = true; }
    inline void setNeedFlush() { m_queuedFlush = true; }

    void StopGlimmer();
    void SpawnRockExplosion( const Int2 &pos, unsigned int health );
    //each cube can have one floating score at a time
    void SpawnScore( unsigned int score, const Int2 &slotpos );
    VideoBuffer &GetVid() { return m_vid; }
    //draw a message box with centered text
    //bDrawBox - draw the box or not
    //in_yOffset - optional y offset for text
    void DrawMessageBoxWithText( const char *pTxt, bool bDrawBox = true, int in_yOffset = 0 ) __attribute__ ((noinline));

    //bg1buffer helpers
    //clears bg1 to White tile
    void ClearBG1();
    //draws bg1
    void FlushBG1();
    TileBuffer<16, 16> &GetBG1Buffer() { return m_bg1buffer; }

    void DrawGrid();
    //special drawing unique to modes
    bool DrawPuzzleModeStuff() __attribute__ ((noinline));
    void DrawBlitzModeStuff( TimeDelta dt ) __attribute__ ((noinline));

    Glimmer m_glimmer;
    //allow up to 4 rock explosions simultaneously
    RockExplosion m_aExplosions[ RockExplosion::MAX_ROCK_EXPLOSIONS ];
    BubbleSpawner m_bubbles;

private:
	//try moving a gem from row1/col1 to row2/col2
	//return if successful
	bool TryMove( int row1, int col1, int row2, int col2 );
    //force this function to be not inline to avoid a bug.  Not sure what the bug is!  But somehow row1/2, col1/2 get garbled otherwise
    static bool FakeTryMove( int row1, int col1, int row2, int col2, GridSlot grid[][NUM_COLS] ) __attribute__ ((noinline));

    //check different parts of the given grid for the given color
    static void TestGridForColor( GridTestInfo &testInfo, const GridSlot grid[][NUM_COLS] ) __attribute__ ((noinline));
    //recursive function to tilt and test grid
    static void TiltAndTestGrid( GridTestInfo &testInfo, int iterations, const GridSlot grid[][NUM_COLS] ) __attribute__ ((noinline));

    bool HasFloatingDots() const;
    void fillPuzzleCube();        
    void UpdateRefill( SystemTime t, TimeDelta dt ) __attribute__ ((noinline));

    CubeID m_cube;
    VideoBuffer m_vid;
    //render bg1 here
    TileBuffer<16, 16> m_bg1buffer;

	CubeState m_state;
	GridSlot m_grid[NUM_ROWS][NUM_COLS];
	Banner m_banner;

	//neighbor info
    int8_t m_neighbors[NUM_SIDES];
	//what time did we start shaking?
    SystemTime m_ShakeTime;
    //how long have we been touching the cube?
    float m_fTouchTime;

    //render based on current fluid level
    //use (-128, 128) range since that matches accelerometer
    Float2 m_curFluidDir;
    Float2 m_curFluidVel;
    //how long have we been not tilting
    float m_idleTimer;

    Intro m_intro;

    float m_timeTillGlimmer;

    float m_stateTime;

    //array of queued clears.
    //clears get queued up in update, then they get drawn before any draws and cleared out
    int8_t m_queuedClearRows[NUM_COLS];
    int8_t m_queuedClearCols[NUM_ROWS];
    uint8_t m_lastTiltDir;

    //do we need to do a bg1 flush?
    bool m_queuedFlush;
    //TODO, need to start using this for other screens
    bool m_dirty;

    FloatingScore m_floatscore;

    friend class Game;
};

#endif
