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

    RockExplosion();
    void Reset();
    void Update( float dt );
    void Draw( VidMode_BG0_SPR_BG1 &vid, int spriteindex );
	
private:
    Vec2 m_pos;
    unsigned int m_animFrame;
};

#endif
