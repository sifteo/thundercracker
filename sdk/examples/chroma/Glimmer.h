/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * timer for timer mode
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _Glimmer_H
#define _Glimmer_H

#include <sifteo.h>

using namespace Sifteo;

class CubeWrapper;

class Glimmer
{
public:
    static const unsigned int NUM_GLIMMER_GROUPS = 7;
    //max at a time
    static const int MAX_GLIMMERS = 4;
    //list of locations to glimmer in order
    static Int2 *GLIMMER_ORDER[NUM_GLIMMER_GROUPS];
    static int NUM_PER_GROUP[NUM_GLIMMER_GROUPS];

    Glimmer();
    void Reset();
    void Update( float dt );
    void Draw( BG1Helper &bg1helper, CubeWrapper *pWrapper );
    inline void Stop() { m_group = NUM_GLIMMER_GROUPS; }
	
private:
    unsigned int m_frame;
    unsigned int m_group;
};

#endif
