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
        const char *chapterTitle,
        const char cutsceneTextStart[][32], unsigned int numCutsceneTextStart,
        const char cutsceneTextEnd[][32], unsigned int numCutsceneTextEnd,
        const char *clue,
        const unsigned int buddies[], unsigned int numBuddies,
        unsigned int numShuffles,
        const Piece startState[kMaxBuddies][NUM_SIDES],
        const Piece endState[kMaxBuddies][NUM_SIDES]);
    
    const char *GetChapterTitle() const;
    void SetChapterTitle(const char *chapterTitle);
    
    void ClearCutsceneTextStart();
    void AddCutsceneTextStart(const char *cutsceneTextStart);
    
    const char *GetCutsceneTextStart(unsigned int cutsceneIndex) const;
    unsigned int GetNumCutsceneTextStart() const;
    
    void ClearCutsceneTextEnd();
    void AddCutsceneTextEnd(const char *cutsceneTextEnd);
    
    const char *GetCutsceneTextEnd(unsigned int cutsceneIndex) const;
    unsigned int GetNumCutsceneTextEnd() const;
    
    const char *GetClue() const;
    void SetClue(const char *clue);
    
    void ClearBuddies();
    void AddBuddy(unsigned int buddyId);
    
    unsigned int GetBuddy(unsigned int buddyIndex) const;
    unsigned int GetNumBuddies() const;
    
    unsigned int GetNumShuffles() const;
    void SetNumShuffles(unsigned int numSuhffles);
    
    const Piece &GetStartState(unsigned int buddy, Sifteo::Cube::Side side) const;
    void SetStartState(unsigned int buddy, Sifteo::Cube::Side side, const Piece &piece);
    
    const Piece &GetEndState(unsigned int buddy, Sifteo::Cube::Side side) const;
    void SetEndState(unsigned int buddy, Sifteo::Cube::Side side, const Piece &piece);
    
private:
    const char *mChapterTitle;
    const char *mCutsceneTextStart[16];
    unsigned int mNumCutsceneTextStart;
    const char *mCutsceneTextEnd[16];
    unsigned int mNumCutsceneTextEnd;
    const char *mClue;
    unsigned int mBuddies[kMaxBuddies];
    unsigned int mNumBuddies;
    unsigned int mNumShuffles;
    Piece mStartState[kMaxBuddies][NUM_SIDES];
    Piece mEndState[kMaxBuddies][NUM_SIDES];    
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
