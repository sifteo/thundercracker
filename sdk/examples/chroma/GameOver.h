/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * timer for timer mode
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _GameOver_H
#define _GameOver_H

#include <sifteo.h>
#include "BG1Helper.h"

using namespace Sifteo;

class GameOver
{
public:
    static const int NUM_ARROWS = 4;
    static const int FRAMES_BETWEEN_ANIMS = 0;

    GameOver();
    void Reset();
    void Update( float dt );
    void Draw( BG1Helper &bg1helper, Cube &cube );
	
private:
    unsigned int m_frame;
};

#endif
