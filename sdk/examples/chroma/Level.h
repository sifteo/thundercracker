/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * timer for timer mode
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _LEVEL_H
#define _LEVEL_H

struct Level
{
public:
    static const unsigned int NUM_LEVELS = 17;

    Level( unsigned int numColors, unsigned int numFixed = 0, unsigned int numRocks = 0 );
	
    static const Level &GetLevel( unsigned int index );

    unsigned int m_numColors;
    unsigned int m_numFixed;
    unsigned int m_numRocks;
};

#endif
