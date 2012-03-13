/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */
 
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BUDDIES_PUZZLE_H_
#define BUDDIES_PUZZLE_H_

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <sifteo/cube.h>
#include "Config.h"
#include "Piece.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Buddies {

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class Puzzle
{
public:
    Puzzle();

    Puzzle(
        const char *title,
        const char cutsceneTextStart[][32], unsigned int numCutsceneTextStart,
        const char cutsceneTextEnd[][32], unsigned int numCutsceneTextEnd,
        const char *clue,
        const unsigned int buddies[], unsigned int numBuddies,
        unsigned int numShuffles,
        const Piece piecesStart[kMaxBuddies][NUM_SIDES],
        const Piece piecesEnd[kMaxBuddies][NUM_SIDES]);
    
    void Reset();
    
    const char *GetTitle() const;
    void SetTitle(const char *title);
    
    void AddCutsceneTextStart(const char *cutsceneTextStart);
    const char *GetCutsceneTextStart(unsigned int cutsceneIndex) const;
    unsigned int GetNumCutsceneTextStart() const;
    
    void AddCutsceneTextEnd(const char *cutsceneTextEnd);
    const char *GetCutsceneTextEnd(unsigned int cutsceneIndex) const;
    unsigned int GetNumCutsceneTextEnd() const;
    
    const char *GetClue() const;
    void SetClue(const char *clue);
    
    unsigned int GetNumShuffles() const;
    void SetNumShuffles(unsigned int numSuhffles);
    
    void AddBuddy(unsigned int buddyId);
    unsigned int GetBuddy(unsigned int buddyIndex) const;
    unsigned int GetNumBuddies() const;
    
    const Piece &GetPieceStart(unsigned int buddy, Sifteo::Cube::Side side) const;
    void SetPieceStart(unsigned int buddy, Sifteo::Cube::Side side, const Piece &piece);
    
    const Piece &GetPieceEnd(unsigned int buddy, Sifteo::Cube::Side side) const;
    void SetPieceEnd(unsigned int buddy, Sifteo::Cube::Side side, const Piece &piece);
    
private:
    const char *mTitle;
    const char *mCutsceneTextStart[16];
    unsigned int mNumCutsceneTextStart;
    const char *mCutsceneTextEnd[16];
    unsigned int mNumCutsceneTextEnd;
    const char *mClue;
    unsigned int mNumShuffles;
    unsigned int mBuddies[kMaxBuddies];
    unsigned int mNumBuddies;
    Piece mPiecesStart[kMaxBuddies][NUM_SIDES];
    Piece mPiecesEnd[kMaxBuddies][NUM_SIDES];    
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
