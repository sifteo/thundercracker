/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * timer for timer mode
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _TIMEKEEPER_H
#define _TIMEKEEPER_H

#include <sifteo.h>

using namespace Sifteo;

class TimeKeeper
{
public:
    static const float TIME_INITIAL;
    //how long does it take the dot to go around?
    static const float TIMER_SPRITE_PERIOD;
    static const int TIMER_STEMS = 14;
    static const unsigned int BLINK_OFF_FRAMES = 7;
    static const unsigned int BLINK_ON_FRAMES = 10;
    static const unsigned int TIMER_POS = 6;
    static const unsigned int TIMER_SPRITE_POS = 48;

	TimeKeeper();

	void Reset();
    void Draw( BG1Helper &bg1helper, VidMode_BG0_SPR_BG1 &vid );
    void Update( float dt );
	void Init( float t );
	
    void DrawMeter( float amount, BG1Helper &bg1helper, VidMode_BG0_SPR_BG1 &vid );
	float getTime() const { return m_fTimer; }

private:
	float m_fTimer;
    unsigned int m_blinkCounter;
};

#endif
