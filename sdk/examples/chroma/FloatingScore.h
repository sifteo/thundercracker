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
    static const unsigned int NUM_POINTS_FRAMES = 4;
    static const float SCORE_FADE_DELAY;
    static const float START_FADING_TIME;

    FloatingScore();

    void Reset();
    void Draw( BG1Helper &bg1helper );
    void Update(float dt);

    void Spawn( unsigned int score, const Vec2 &pos );
    inline bool IsActive() const { return m_fLifetime >= 0.0f; }
private:
    float m_fLifetime;
    unsigned int m_score;
    Vec2 m_pos;
};

#endif
