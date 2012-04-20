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

#include <sifteo/asset.h>
#include <sifteo/cube.h>
#include <sifteo/video.h>
#include "BuddyId.h"
#include "GameState.h"
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
    bool Update(float dt); // TODO: Hacky hook for blink event
    
    // TODO: This is only here for menu access... breaks encapsulation :(
    Sifteo::VideoBuffer &GetVideoBuffer() { return mVideoBuffer; }
    
    // Drawing
    void DrawClear();
    void DrawFlush();
    
    void DrawBuddy();
    
    void DrawBackground(const Sifteo::AssetImage &asset);
    void DrawBackgroundPartial(
        Sifteo::Int2 position,
        Sifteo::Int2 offset,
        Sifteo::Int2 size,
        const Sifteo::AssetImage &asset);
    void ScrollBackground(Sifteo::Int2 position);
    
    void DrawSprite(
        int spriteIndex,
        Sifteo::Int2 position,
        const Sifteo::PinnedAssetImage &asset, unsigned int assetFrame = 0);
    
    void DrawUiAsset(
        Sifteo::Int2 position,
        const Sifteo::AssetImage &asset, unsigned int assetFrame = 0);
    void DrawUiAssetPartial(
        Sifteo::Int2 position,
        Sifteo::Int2 offset,
        Sifteo::Int2 size,
        const Sifteo::AssetImage &asset, unsigned int assetFrame = 0);
    void DrawUiText(
        Sifteo::Int2 position,
        const Sifteo::AssetImage &assetFont,
        const char *text);
    void ScrollUi(Sifteo::Int2 position);
    
    Sifteo::PCubeID GetId();
    
    // Enable/Disable
    bool IsEnabled() const;
    void Enable(Sifteo::PCubeID cubeId);
    void Disable();
    
    // Buddy
    BuddyId GetBuddyId() const;
    void SetBuddyId(BuddyId buddyId);
    
    // Pieces
    const Piece &GetPiece(Sifteo::Side side) const;
    void SetPiece(Sifteo::Side side, const Piece &piece);
    
    const Piece &GetPieceSolution(Sifteo::Side side) const;
    void SetPieceSolution(Sifteo::Side, const Piece &piece);
    
    Sifteo::Int2 GetPieceOffset(Sifteo::Side side) const;
    void SetPieceOffset(Sifteo::Side side, Sifteo::Int2 offset);
    
    void StartPieceBlinking(Sifteo::Side side);
    void StopPieceBlinking();
    
    // Tilt
    Sifteo::Byte2 GetTiltState() const;
    Sifteo::Byte3 GetAccelState() const;
    
    // Bump
    void StartBump(Sifteo::Side side);
    
    // State
    bool IsSolved() const;
    bool IsTouching() const;
    
private:
    void DrawPiece(const Piece &piece, Sifteo::Side side);
    
    Sifteo::VideoBuffer mVideoBuffer;
    Sifteo::CubeID mCubeId;
    
    bool mEnabled;
    BuddyId mBuddyId;
    
    Piece mPieces[Sifteo::NUM_SIDES];
    Piece mPiecesSolution[Sifteo::NUM_SIDES];
    
    Sifteo::Int2 mPieceOffsets[Sifteo::NUM_SIDES];
    
    Sifteo::Side mPieceBlinking;
    float mPieceBlinkTimer;
    bool mPieceBlinkingOn;
    
    float mBumpTimer;
    Sifteo::Side mBumpSide;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
