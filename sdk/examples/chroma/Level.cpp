/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * timer for the game
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "Level.h"


static const Level s_levels[ Level::NUM_LEVELS ] =
{
    Level( 3 ),
    Level( 4 ),
    Level( 4, 1 ),
    Level( 5, 1 ),
    Level( 5, 2 ),
    Level( 5, 2, 1 ),
    Level( 6, 2 ),
    Level( 6, 2 ),
    Level( 6, 2, 1 ),
    Level( 7, 3 ),
    Level( 7, 3 ),
    Level( 7, 3, 1 ),
    Level( 8, 3 ),
    Level( 8, 3 ),
    Level( 8, 4 ),
    Level( 8, 4, 1 ),
    Level( 8, 4, 2 ),
};


Level::Level( unsigned int numColors, unsigned int numFixed, unsigned int numRocks ) :
    m_numColors( numColors ), m_numFixed( numFixed ), m_numRocks( numRocks )
{
}



const Level &Level::GetLevel( unsigned int index )
{
    if( index >= NUM_LEVELS )
        return s_levels[ NUM_LEVELS - 1 ];

    return s_levels[ index ];
}
