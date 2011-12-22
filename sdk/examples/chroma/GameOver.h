/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * timer for timer mode
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _GAMEOVER_H
#define _GAMEOVER_H

#include <sifteo.h>

using namespace Sifteo;

class GameOver
{
public:
    static const int NUM_ARROWS = 4;
    static const int FRAMES_BETWEEN_ANIMS = 0;

    GameOver();
    void Reset();
    void Update( float dt );
    void Draw( Cube &cube );
	
private:
    unsigned int m_frame;
};

#endif
