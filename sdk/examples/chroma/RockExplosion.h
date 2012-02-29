/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Logic for handling rock explosions
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _ROCKEXPLOSION_H
#define _ROCKEXPLOSION_H

#include <sifteo.h>

using namespace Sifteo;

class RockExplosion
{
public:
    static const int MAX_ROCK_EXPLOSIONS = 4;
    static const float FRAMES_PER_SECOND;

    RockExplosion();
    void Reset();
    void Update();
    void Draw( VidMode_BG0_SPR_BG1 &vid, int spriteindex );
    void Spawn( const Vec2 &pos, int whichpiece );

    inline bool isUnused() const { return m_pos.x < 0; }
    inline unsigned int getAnimFrame() const { return m_animFrame; }
	
private:
    Vec2 m_pos;
    unsigned int m_animFrame;
    float m_startTime;
};

#endif
