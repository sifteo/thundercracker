/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SIFTEO_BUDDIES_CUBEWRAPPER_H_
#define SIFTEO_BUDDIES_CUBEWRAPPER_H_

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <sifteo/cube.h>
#include "Config.h"
#include "Piece.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace Buddies {

///////////////////////////////////////////////////////////////////////////////////////////////////
/// CubeWrapper contains state for each Sifteo::Cube. Instead of exposing the Cube directly, this
/// class manages all access so it can enforce the game logic.
///////////////////////////////////////////////////////////////////////////////////////////////////

class CubeWrapper
{
public:
    CubeWrapper();
    
    void Reset();
    void Update(float dt);
    
    void DrawBuddy();
    void DrawShuffleUi(GameState shuffleState, float shuffleScoreTime);
    void DrawTitleCard(const char *text);
    void DrawTextBanner(const char *text);
    void DrawHintBar(Sifteo::Cube::Side side);
    
    void EnableBg0SprBg1Video(); // HACK!
    void ClearBg1(); // HACK!
    
    // Asset Loading
    bool IsLoadingAssets();
    void LoadAssets();
    void DrawLoadingAssets();
    
    // Enable/Disable
    bool IsEnabled() const;
    void Enable(Sifteo::Cube::ID cubeId, unsigned int buddyId);
    void Disable();
    
    // Pieces
    const Piece &GetPiece(Sifteo::Cube::Side side) const;
    void SetPiece(Sifteo::Cube::Side side, const Piece &piece);
    
    const Piece &GetPieceSolution(Sifteo::Cube::Side side) const;
    void SetPieceSolution(Sifteo::Cube::Side, const Piece &piece);
    
    int GetPieceOffset(Sifteo::Cube::Side side) const;
    void SetPieceOffset(Sifteo::Cube::Side side, int offset);
    
    // State
    bool IsSolved() const;
    bool IsTouching() const;
    
private:
    Sifteo::VidMode_BG0_SPR_BG1 Video();
    void DrawPiece(const Piece &piece, Sifteo::Cube::Side side);
    void DrawBanner(const Sifteo::AssetImage &asset);
    void DrawScoreBanner(const Sifteo::AssetImage &asset, int minutes, int seconds);
    
    Sifteo::Cube mCube;
    bool mEnabled;
    unsigned int mBuddyId;
    Piece mPieces[NUM_SIDES];
    Piece mPiecesSolution[NUM_SIDES];
    int mPieceOffsets[NUM_SIDES];
    float mPieceAnimT;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
