/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * handles drawing of score floating up
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _FLOATINGSCORE_H
#define _FLOATINGSCORE_H

#include <sifteo.h>

using namespace Sifteo;

class FloatingScore
{
public:
    //static const unsigned int NUM_POINTS_FRAMES = 4;
    static const float SCORE_FADE_DELAY;
    //static const float START_FADING_TIME;

    FloatingScore();

    void Reset();
    void Draw( BG1Helper &bg1helper ) __attribute__ ((noinline));
    void Update(float dt);

    void Spawn( unsigned int score, const Int2 &pos );
    inline bool IsActive() const { return m_fLifetime >= 0.0f; }
private:
    float m_fLifetime;
    unsigned int m_score;
    Int2 m_pos;
};

#endif
