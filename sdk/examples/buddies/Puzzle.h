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
#include "BuddyId.h"
#include "Config.h"
#include "CutsceneLine.h"
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
        unsigned int book,
        const char *title,
        const char cutsceneTextStart[][32], unsigned int numCutsceneTextStart,
        const char cutsceneTextEnd[][32], unsigned int numCutsceneTextEnd,
        const char *clue,
        const BuddyId buddies[], unsigned int numBuddies,
        unsigned int numShuffles,
        const Piece piecesStart[kMaxBuddies][NUM_SIDES],
        const Piece piecesEnd[kMaxBuddies][NUM_SIDES]);
    
    void Reset();
    
    unsigned int GetBook() const;
    void SetBook(unsigned int book);
    
    const char *GetTitle() const;
    void SetTitle(const char *title);
    
    const char *GetClue() const;
    void SetClue(const char *clue);
    
    void AddCutsceneLineStart(const CutsceneLine &line);
    const CutsceneLine &GetCutsceneLineStart(unsigned int cutsceneIndex) const;
    unsigned int GetNumCutsceneLineStart() const;
    
    void AddCutsceneLineEnd(const CutsceneLine &line);
    const CutsceneLine &GetCutsceneLineEnd(unsigned int cutsceneIndex) const;
    unsigned int GetNumCutsceneLineEnd() const;
    
    unsigned int GetCutsceneEnvironment() const;
    void SetCutsceneEnvironment(unsigned int cutsceneEnvionment);
    
    unsigned int GetNumShuffles() const;
    void SetNumShuffles(unsigned int numSuhffles);
    
    void AddBuddy(BuddyId buddyId);
    BuddyId GetBuddy(unsigned int buddyIndex) const;
    unsigned int GetNumBuddies() const;
    
    const Piece &GetPieceStart(unsigned int buddy, Sifteo::Cube::Side side) const;
    void SetPieceStart(unsigned int buddy, Sifteo::Cube::Side side, const Piece &piece);
    
    const Piece &GetPieceEnd(unsigned int buddy, Sifteo::Cube::Side side) const;
    void SetPieceEnd(unsigned int buddy, Sifteo::Cube::Side side, const Piece &piece);
    
private:
    unsigned char mBook : 4;
    const char *mTitle;
    const char *mClue;
    CutsceneLine mCutsceneLineStart[8];
    CutsceneLine mCutsceneLineEnd[8];
    unsigned char mBuddies[kMaxBuddies];
    unsigned char mNumCutsceneLineStart : 3;
    unsigned char mNumCutsceneLineEnd : 3;
    unsigned char mCutsceneEnvironemnt : 3;
    unsigned char mNumShuffles : 8;
    unsigned char mNumBuddies : 5;
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
