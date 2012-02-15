/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * timer for the game
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "Puzzle.h"
#include "PuzzleData.h"

PuzzleCubeData::PuzzleCubeData( unsigned int *pValues )
{
    for( int i = 0; i < CubeWrapper::NUM_ROWS; i++ )
    {
        for( int j = 0; j < CubeWrapper::NUM_COLS; j++ )
        {
            m_aData[i][j] = *( pValues + ( CubeWrapper::NUM_COLS * i ) + j );
        }
    }
}


Puzzle::Puzzle( const char *pName, const char *pInstr, const PuzzleCubeData *pData, unsigned int numCubes, bool bTiltAllowed ) :
    m_pName( pName ), m_pInstr( pInstr ), m_pData( pData ), m_numCubes( numCubes ), m_bTiltAllowed( bTiltAllowed )
{
}

