/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * timer for timer mode
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _INTRO_H
#define _INTRO_H

#include <sifteo.h>
#include "TimeKeeper.h"
#include "banner.h"

using namespace Sifteo;

class CubeWrapper;

class Intro
{
public:
    typedef enum
    {
        STATE_BALLEXPLOSION,
        STATE_READY,
        STATE_SET,
        STATE_GO,
        STATE_CNT,
    } IntroState;

    static const int NUM_ARROWS = 4;
    static const int NUM_TOTAL_EXPLOSION_FRAMES = 5;
    static const float READYSETGO_BANNER_TIME;

    Intro();
    void Reset( bool ingamereset = false );
    bool Update( SystemTime t, TimeDelta dt, Banner &banner );
    //return whether we touched bg1 or not
    bool Draw( TimeKeeper &timer, BG1Helper &bg1helper, VidMode_BG0_SPR_BG1 &vid, CubeWrapper *pWrapper );
    inline IntroState getState() const { return m_state; }
	
private:
    Int2 LerpPosition( Int2 &start, Int2 &end, float timePercent );

    IntroState m_state;
	float m_fTimer;
};

#endif
