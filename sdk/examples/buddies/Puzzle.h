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
        const char *clue,
        const BuddyId cutsceneBuddiesStart[], unsigned int numCutsceneBuddiesStart,
        const CutsceneLine cutsceneLineStart[], unsigned int numCutsceneLineStart,
        const BuddyId cutsceneBuddiesEnd[], unsigned int numCutsceneBuddiesEnd,
        const CutsceneLine cutsceneLineEnd[], unsigned int numCutsceneLineEnd,
        unsigned int cutsceneEnvironment,
        const BuddyId buddies[], unsigned int numBuddies,
        unsigned int numShuffles,
        const Piece piecesStart[][NUM_SIDES],
        const Piece piecesEnd[][NUM_SIDES]);
    
    void Reset();
    
    unsigned int GetBook() const;
    void SetBook(unsigned int book);
    
    const char *GetTitle() const;
    void SetTitle(const char *title);
    
    const char *GetClue() const;
    void SetClue(const char *clue);
    
    void AddCutsceneBuddyStart(BuddyId buddyId);
    BuddyId GetCutsceneBuddyStart(unsigned int buddyIndex) const;
    unsigned int GetNumCutsceneBuddiesStart() const;
    
    void AddCutsceneLineStart(const CutsceneLine &line);
    const CutsceneLine &GetCutsceneLineStart(unsigned int cutsceneIndex) const;
    unsigned int GetNumCutsceneLineStart() const;
    
    void AddCutsceneBuddyEnd(BuddyId buddyId);
    BuddyId GetCutsceneBuddyEnd(unsigned int buddyIndex) const;
    unsigned int GetNumCutsceneBuddiesEnd() const;
    
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
    // TODO: Optimzie for size better
    unsigned char mBook : 4;
    const char *mTitle;
    const char *mClue;
    unsigned char mCutsceneBuddiesStart[2];
    unsigned char mCutsceneBuddiesEnd[2];
    CutsceneLine mCutsceneLineStart[8];
    CutsceneLine mCutsceneLineEnd[8];
    unsigned char mBuddies[kMaxBuddies];
    unsigned char mNumCutsceneBuddiesStart : 2;
    unsigned char mNumCutsceneBuddiesEnd : 2;
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
