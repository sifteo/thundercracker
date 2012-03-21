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

#include <sifteo/BG1Helper.h>
#include <sifteo/cube.h>
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
    void Update(float dt);
    
    // Drawing
    void DrawClear();
    void DrawFlush();
    bool DrawNeedsSync();
    
    void DrawBuddy();
    
    void DrawBackground(const Sifteo::AssetImage &asset);
    void DrawBackgroundPartial(
        const Sifteo::Vec2 &position,
        const Sifteo::Vec2 &offset,
        const Sifteo::Vec2 &size,
        const Sifteo::AssetImage &asset);
    void ScrollBackground(const Sifteo::Vec2 &position);
    
    void DrawUiAsset(
        const Sifteo::Vec2 &position,
        const Sifteo::AssetImage &asset, unsigned int assetFrame = 0);
    void DrawUiAssetPartial(
        const Sifteo::Vec2 &position,
        const Sifteo::Vec2 &offset,
        const Sifteo::Vec2 &size,
        const Sifteo::AssetImage &asset, unsigned int assetFrame = 0);
    void DrawUiText(
        const Sifteo::Vec2 &position,
        const Sifteo::AssetImage &assetFont,
        const char *text);
    void ScrollUi(const Sifteo::Vec2 &position);
    
    // Special-Case Cutscene Stuff
    void UpdateCutscene(int jumpChanceA, int jumpChanceB);
    void DrawCutsceneShuffle(const Sifteo::Vec2 &scroll);
    void DrawCutsceneStory(const char *text);
    
    // Asset Loading
    bool IsLoadingAssets();
    void LoadAssets();
    void DrawLoadingAssets();
    
    Sifteo::Cube::ID GetId();
    
    // Enable/Disable
    bool IsEnabled() const;
    void Enable(Sifteo::Cube::ID cubeId);
    void Disable();
    
    // Buddy
    unsigned int GetBuddyId() const;
    void SetBuddyId(unsigned int buddyId);
    
    // Pieces
    const Piece &GetPiece(Sifteo::Cube::Side side) const;
    void SetPiece(Sifteo::Cube::Side side, const Piece &piece);
    
    const Piece &GetPieceSolution(Sifteo::Cube::Side side) const;
    void SetPieceSolution(Sifteo::Cube::Side, const Piece &piece);
    
    const Vec2 &GetPieceOffset(Sifteo::Cube::Side side) const;
    void SetPieceOffset(Sifteo::Cube::Side side, const Vec2 &offset);
    
    void StartPieceBlinking(Sifteo::Cube::Side side);
    void StopPieceBlinking();
    
    // Tilt
    Sifteo::Cube::TiltState GetTiltState() const;
    Sifteo::Vec2 GetAccelState() const;
    
    // State
    bool IsSolved() const;
    bool IsTouching() const;
    
private:
    Sifteo::VidMode_BG0_SPR_BG1 Video();
    
    void DrawPiece(const Piece &piece, Sifteo::Cube::Side side);
    void UpdateCutsceneSpriteJump(bool &cutsceneSpriteJump, int upChance, int downChance);

    Sifteo::Cube mCube;
    BG1Helper mBg1Helper;
    
    bool mEnabled;
    unsigned int mBuddyId;
    Piece mPieces[NUM_SIDES];
    Piece mPiecesSolution[NUM_SIDES];
    Sifteo::Vec2 mPieceOffsets[NUM_SIDES];
    Sifteo::Cube::Side mPieceBlinking;
    float mPieceBlinkTimer;
    bool mPieceBlinkingOn;
    
    // Cutscene Jump Animation
    Sifteo::Math::Random mCutsceneSpriteJumpRandom;
    bool mCutsceneSpriteJump0;
    bool mCutsceneSpriteJump1;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
