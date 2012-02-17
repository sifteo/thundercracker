/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * timer for the game
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "Puzzle.h"
#include "PuzzleData.h"

PuzzleCubeData::PuzzleCubeData( uint8_t *pValues )
{
    for( int i = 0; i < CubeWrapper::NUM_ROWS; i++ )
    {
        for( int j = 0; j < CubeWrapper::NUM_COLS; j++ )
        {
            m_aData[i][j] = *( pValues + ( CubeWrapper::NUM_COLS * i ) + j );
            ASSERT( m_aData[i][j] >= 0 && m_aData[i][j] <= GridSlot::NUM_COLORS_INCLUDING_SPECIALS )
        }
    }
}


Puzzle::Puzzle( const char *pName, const char *pInstr, unsigned int dataIndex, unsigned int numCubes, bool bTiltAllowed ) :
    m_pName( pName ), m_pInstr( pInstr ), m_dataIndex( dataIndex ), m_numCubes( numCubes ), m_bTiltAllowed( bTiltAllowed )
{
}


const Puzzle *Puzzle::GetPuzzle( unsigned int index )
{
    unsigned int numPuzzles = GetNumPuzzles();

    if( index >= numPuzzles )
        return NULL;

    return &s_puzzles[ index ];
}


unsigned int Puzzle::GetNumPuzzles()
{
    return sizeof( s_puzzles ) / sizeof( Puzzle );
}


const PuzzleCubeData *Puzzle::getCubeData( unsigned int cubeIndex ) const
{
    if( cubeIndex >= m_numCubes )
        return NULL;

    return &s_puzzledata[ m_dataIndex + cubeIndex ];
}
