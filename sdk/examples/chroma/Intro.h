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

class Intro
{
public:
    static const int NUM_ARROWS = 4;
    static const float INTRO_ARROW_TIME;
    static const float INTRO_TIMEREXPANSION_TIME;

    Intro();
    void Reset();
    void Update( float dt );
    void Draw( TimeKeeper &timer, BG1Helper &bg1helper, Cube &cube );
	
private:
    Vec2 LerpPosition( Vec2 &start, Vec2 &end, float timePercent );

	float m_fTimer;
};

#endif
