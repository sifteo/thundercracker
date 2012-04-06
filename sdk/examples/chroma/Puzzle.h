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
    PuzzleCubeData( uint8_t *pValues );

    uint8_t m_aData[CubeWrapper::NUM_ROWS][CubeWrapper::NUM_COLS];
};


struct Puzzle
{
public:
    static const unsigned int MAX_PUZZLE_CUBES = 6;

    Puzzle( const char *pName, const char *pInstr, unsigned int dataIndex, unsigned int numCubes, bool bTiltAllowed = true );
	
    static const Puzzle *GetPuzzle( unsigned int index );
    static unsigned int GetNumPuzzles();

    const PuzzleCubeData *getCubeData( unsigned int cubeIndex ) const;
    //given puzzle, return chapter index
    static unsigned int GetChapter( unsigned int puzzle );
    static unsigned int GetNumChapters();
    static unsigned int GetNumPuzzlesInChapter( unsigned int chapter );
    //index of first puzzle in the given chapter
    static unsigned int GetPuzzleOffset( unsigned int chapter );

    const char *m_pName;
    const char *m_pInstr;
    unsigned int m_dataIndex;
    unsigned int m_numCubes;
    bool m_bTiltAllowed;


};

#endif
