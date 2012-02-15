/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * timer for timer mode
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _PUZZLE_H
#define _PUZZLE_H

#include "cubewrapper.h"


//dot data for one cube
struct PuzzleCubeData
{
public:
    PuzzleCubeData( unsigned int *pValues );

    unsigned int m_aData[CubeWrapper::NUM_ROWS][CubeWrapper::NUM_COLS];
};


struct Puzzle
{
public:
    static const unsigned int MAX_PUZZLE_CUBES = 6;

    Puzzle( const char *pName, const char *pInstr, const PuzzleCubeData *pData, unsigned int numCubes, bool bTiltAllowed = true );
	
    static const Puzzle *GetPuzzle( unsigned int index );

    const PuzzleCubeData *getCubeData( unsigned int cubeIndex ) const;

    const char *m_pName;
    const char *m_pInstr;
    const PuzzleCubeData *m_pData;
    unsigned int m_numCubes;
    bool m_bTiltAllowed;


};

#endif
