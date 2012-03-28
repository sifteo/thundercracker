/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _GRIDSLOT_H
#define _GRIDSLOT_H

#include <sifteo.h>

using namespace Sifteo;
class CubeWrapper;

//space for a gem
class GridSlot
{
public:
    static const unsigned int NUM_COLORS = 8;
    static const unsigned int NUM_SPAWN_FRAMES = 4;
    static const unsigned int MULT_SPRITE_ID = 2;
    static const unsigned int MULT_SPRITE_NUM_ID = 1;
    static const float MULTIPLIER_LIGHTNING_PERIOD;
    static const float MULTIPLIER_NUMBER_PERIOD;
    //what proportion of MULTIPLIER_NUMBER_PERIOD is the number displayed
    static const float MULTIPLIER_NUMBER_PERCENTON;

    //these are special dots
    enum
    {
        HYPERCOLOR = NUM_COLORS,
        ROCKCOLOR,
        RAINBALLCOLOR,
        AFTERLASTSPECIAL,
        NUM_SPECIALS = AFTERLASTSPECIAL - NUM_COLORS
    };

    static const unsigned int NUM_COLORS_INCLUDING_SPECIALS = NUM_COLORS + NUM_SPECIALS;

    static const AssetImage *TEXTURES[ NUM_COLORS ];
    static const AssetImage *EXPLODINGTEXTURES[ NUM_COLORS ];
    static const AssetImage *FIXED_TEXTURES[ NUM_COLORS ];
    static const AssetImage *FIXED_EXPLODINGTEXTURES[ NUM_COLORS ];
    static const AssetImage *SPECIALTEXTURES[ NUM_SPECIALS ];
    static const AssetImage *SPECIALEXPLODINGTEXTURES[ NUM_SPECIALS ];


    static const unsigned int NUM_QUANTIZED_TILT_VALUES = 7;
    static const unsigned int NUM_ROLL_FRAMES;
    //static const unsigned int NUM_IDLE_FRAMES;

    static const float MARK_SPREAD_DELAY;
    static const float MARK_BREAK_DELAY;
    static const float MARK_EXPLODE_DELAY;
    static const float EXPLODE_FRAME_LEN;
    static const int NUM_EXPLODE_FRAMES = 7;
    static const int NUM_FRAMES_PER_ROLL_ANIM_FRAME = 3;
    static const unsigned int NUM_FRAMES_PER_FIXED_ANIM_FRAME = 3;
    static const unsigned int NUM_FIXED_FRAMES = 5;
    static const unsigned int MAX_ROCK_HEALTH = 4;

	typedef enum 
	{
        //only used for timer mode independent spawning currently
        STATE_SPAWNING,
		STATE_LIVING,
		STATE_MARKED,
		STATE_EXPLODING,
		STATE_GONE,
    } SLOT_STATE;

    typedef enum
    {
        MOVESTATE_STATIONARY,
        MOVESTATE_PENDINGMOVE,
        MOVESTATE_MOVING,
        MOVESTATE_FINISHINGMOVE,
        MOVESTATE_FIXEDATTEMPT,
    } MOVE_STATE;


	GridSlot();

	void Init( CubeWrapper *pWrapper, unsigned int row, unsigned int col ); 
	//draw self on given vid at given vec
    void Draw( VidMode_BG0_SPR_BG1 &vid, BG1Helper &bg1helper, Float2 &tiltState );
    void DrawIntroFrame( VidMode_BG0 &vid, unsigned int frame );
    void Update(float t);
    bool isAlive() const { return m_state == STATE_LIVING; }
    bool isEmpty() const { return m_state == STATE_GONE; }
	bool isMarked() const { return ( m_state == STATE_MARKED || m_state == STATE_EXPLODING ); }
    bool isTiltable() const { return ( m_state == STATE_LIVING || m_state == STATE_MARKED ); }
    bool isMatchable() const { return isAlive() || isMarked(); }
    void setEmpty() { m_state = STATE_GONE; m_bFixed = false; }
	unsigned int getColor() const { return m_color; }
    void FillColor( unsigned int color, bool bSetSpawn = false );
    bool matchesColor( unsigned int color ) const;

	void mark();
	void spread_mark();
	void explode();
	void die();

	bool IsFixed() const { return m_bFixed; }
	void MakeFixed() { m_bFixed = true; }
    void setFixedAttempt();

    inline bool IsSpecial() const { return m_color >= NUM_COLORS; }

	//copy color and some other attributes from target.  Used when tilting
	void TiltFrom(GridSlot &src);
	//if we have a move pending, start it
	void startPendingMove();

    void DamageRock();
    inline unsigned int getMultiplier() { return m_multiplier; }
    inline void setMultiplier( unsigned int mult ) { m_multiplier = mult; }
    void UpMultiplier();
    //morph from rainball to given color
    void RainballMorph( unsigned int color );
    void Infect() { m_bWasInfected = true; }
    //bubble is bumping this chromit, tilt it in the given direction
    void Bump( const Float2 &dir );

private:
	void markNeighbor( int row, int col );
    void hurtNeighboringRock( int row, int col );
    //given tilt state, return our desired frame
    unsigned int GetTiltFrame( Float2 &tiltState, Vec2 &quantized ) const;
    const AssetImage &GetTexture() const;
    const AssetImage &GetExplodingTexture() const;
    const AssetImage &GetSpecialTexture() const;
    const AssetImage &GetSpecialExplodingTexture() const;
    unsigned int GetSpecialFrame();
    //convert from [-128, 128] to [0, 6] via non-linear quantization
    unsigned int QuantizeTiltValue( float value ) const;
    //get the rolling frame of the given index
    unsigned int GetRollingFrame( unsigned int index );
    //unsigned int GetIdleFrame();
    unsigned int GetFixedFrame( unsigned int index );

	SLOT_STATE m_state;
    MOVE_STATE m_Movestate;
	unsigned int m_color;
	float m_eventTime;
	CubeWrapper *m_pWrapper;
	unsigned int m_row;
	unsigned int m_col;

	//current position in 16x16 grid for use when moving
	Vec2 m_curMovePos;

	unsigned int m_score;
	//fixed dot
	bool		 m_bFixed;
    //used to tell if this dot was a rainball (for a special animation)
    bool         m_bWasRainball;
    //was this infected by a hyperball?
    bool         m_bWasInfected;

    //only fixed dots can have multipliers
    unsigned int m_multiplier;

	unsigned int m_animFrame;
    unsigned int m_RockHealth;
    //x,y coordinates of our last frame, so we don't make any large jumps
    Vec2 m_lastFrameDir;
};


#endif
