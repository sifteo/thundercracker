/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * timer for timer mode
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _INTRO_H
#define _INTRO_H

#include <sifteo.h>
#include "TimeKeeper.h"

using namespace Sifteo;

class CubeWrapper;

class Intro
{
public:
    static const int NUM_ARROWS = 4;
    static const float INTRO_ARROW_TIME;
    static const float INTRO_TIMEREXPANSION_TIME;
    static const float INTRO_BALLEXPLODE_TIME;
    static const int NUM_TOTAL_EXPLOSION_FRAMES = 5;

    Intro();
    void Reset( bool ingamereset = false );
    bool Update( float dt );
    //return whether we touched bg1 or not
    bool Draw( TimeKeeper &timer, BG1Helper &bg1helper, Cube &cube, CubeWrapper *pWrapper );
	
private:
    Vec2 LerpPosition( Vec2 &start, Vec2 &end, float timePercent );

	float m_fTimer;
};

#endif
