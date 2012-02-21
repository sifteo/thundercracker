/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "CubeWrapper.h"
#include <sifteo.h>
#include "Config.h"
#include "assets.gen.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace Buddies {

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

///////////////////////////////////////////////////////////////////////////////////////////////////
// || Various convenience functions...
// \/ 
///////////////////////////////////////////////////////////////////////////////////////////////////

const AssetImage &GetBuddyFaceBackgroundAsset(int buddyId)
{
    switch (buddyId)
    {
        default:
        case 0: return BuddyFaceBackground0;
        case 1: return BuddyFaceBackground1;
        case 2: return BuddyFaceBackground2;
        case 3: return BuddyFaceBackground3;
        case 4: return BuddyFaceBackground4;
        case 5: return BuddyFaceBackground5;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

const PinnedAssetImage &GetBuddyFacePartsAsset(int buddyId)
{
    switch (buddyId)
    {
        default:
        case 0: return BuddyFaceParts0;
        case 1: return BuddyFaceParts1;
        case 2: return BuddyFaceParts2;
        case 3: return BuddyFaceParts3;
        case 4: return BuddyFaceParts4;
        case 5: return BuddyFaceParts5;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Vec2 GetHintBarPoint(Cube::Side side)
{
    ASSERT(side >= 0 && side < NUM_SIDES);
    
    switch (side)
    {
        default:
        case SIDE_TOP:    return Vec2( 0,  0);
        case SIDE_LEFT:   return Vec2( 0,  0);
        case SIDE_BOTTOM: return Vec2( 0, 11);
        case SIDE_RIGHT:  return Vec2(11,  0);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

const AssetImage &GetHintBarAsset(Cube::ID cubeId, Cube::Side side)
{
    ASSERT(side >= 0 && side < NUM_SIDES);
    
    switch (side)
    {
        default:
        case SIDE_TOP:    return cubeId == 0 ? HintBarBlueTop    : HintBarOrangeTop;
        case SIDE_LEFT:   return cubeId == 0 ? HintBarBlueLeft   : HintBarOrangeLeft;
        case SIDE_BOTTOM: return cubeId == 0 ? HintBarBlueBottom : HintBarOrangeBottom;
        case SIDE_RIGHT:  return cubeId == 0 ? HintBarBlueRight  : HintBarOrangeRight;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// ||
// \/ Static data
///////////////////////////////////////////////////////////////////////////////////////////////////

const Vec2 kPartPositions[NUM_SIDES] =
{
    Vec2(32, -8),
    Vec2(-8, 32),
    Vec2(32, 72),
    Vec2(72, 32),
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

CubeWrapper::CubeWrapper()
    : mCube()
    , mEnabled(false)
    , mBuddyId(0)
    , mPieces()
    , mPiecesSolution()
    , mPieceOffsets()
    , mPieceAnimT(0.0f)
    , mRandom()
    , mCutsceneSpriteJump0(false)
    , mCutsceneSpriteJump1(false)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::Reset()
{
    for (unsigned int i = 0; i < arraysize(mPieceOffsets); ++i)
    {
        mPieceOffsets[i] = 0;
    }
    
    mPieceAnimT = 0.0f;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::Update(float dt)
{
    mPieceAnimT += dt;
    while (mPieceAnimT > kPieceAnimPeriod)
    {
        mPieceAnimT -= kPieceAnimPeriod;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawBuddy()
{
    ASSERT(IsEnabled());
    
    EnableBg0SprBg1Video();
    Video().BG0_drawAsset(Vec2(0, 0), GetBuddyFaceBackgroundAsset(mBuddyId));
    
    DrawPiece(mPieces[SIDE_TOP], SIDE_TOP);
    DrawPiece(mPieces[SIDE_LEFT], SIDE_LEFT);
    DrawPiece(mPieces[SIDE_BOTTOM], SIDE_BOTTOM);
    DrawPiece(mPieces[SIDE_RIGHT], SIDE_RIGHT);
}  

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawBuddyWithStoryHint(Sifteo::Cube::Side side, bool blink)
{
    ASSERT(IsEnabled());
    
    EnableBg0SprBg1Video();
    Video().BG0_drawAsset(Vec2(0, 0), GetBuddyFaceBackgroundAsset(mBuddyId));
    
    for (unsigned int i = 0; i < NUM_SIDES; ++i)
    {
        if (side != int(i) || !blink)
        {
            DrawPiece(mPieces[i], i);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawShuffleUi(
    GameState shuffleState,
    float shuffleScoreTime,
    int shuffleHintPiece0,
    int shuffleHintPiece1)
{
    ASSERT(kGameMode == GAME_MODE_SHUFFLE);
    
    switch (shuffleState)
    {
        case GAME_STATE_SHUFFLE_SHAKE_TO_SCRAMBLE:
        {
            DrawBanner(mCube.id() == 0 ? ShakeToShuffleBlue :  ShakeToShuffleOrange);
            break;
        }
        case GAME_STATE_SHUFFLE_UNSCRAMBLE_THE_FACES:
        {
            DrawBanner(mCube.id() == 0 ? UnscrableTheFacesBlue : UnscrableTheFacesOrange);
            break;
        }
        case GAME_STATE_SHUFFLE_PLAY:
        {
            if (shuffleHintPiece0 != -1 && (shuffleHintPiece0 / NUM_SIDES) == mCube.id())
            {
                DrawHintBar(shuffleHintPiece0 % NUM_SIDES);
            }
            else if (shuffleHintPiece1 != -1 && (shuffleHintPiece1 / NUM_SIDES) == mCube.id())
            {
                DrawHintBar(shuffleHintPiece1 % NUM_SIDES);
            }
            else if (IsSolved())
            {
                DrawBanner(mCube.id() == 0 ? FaceCompleteBlue : FaceCompleteOrange);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_SCORE:
        {
            if (mCube.id() == 0)
            {
                int minutes = int(shuffleScoreTime) / 60;
                int seconds = int(shuffleScoreTime - (minutes * 60.0f));
                DrawScoreBanner(ScoreTimeBlue, minutes, seconds);
            }
            else
            {
                DrawBanner(ShakeToShuffleOrange);
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawClue(const char *text, bool moreHints)
{
    BG1Helper bg1helper(mCube);
    
    if (moreHints)
    {
        bg1helper.DrawAsset(Vec2(0, 3), MoreHints);
        if (text != NULL)
        {
            bg1helper.DrawText(Vec2(2, 4), Font, text);
        }
    }
    else
    {
        bg1helper.DrawAsset(Vec2(0, 3), ClueText);
        if (text != NULL)
        {
            bg1helper.DrawText(Vec2(2, 4), Font, text);
        }
    }
    
    bg1helper.Flush();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawTextBanner(const char *text)
{
    ASSERT(text != NULL);
    
    BG1Helper bg1helper(mCube);
    bg1helper.DrawAsset(Vec2(0, 0), BannerEmpty);
    bg1helper.DrawText(Vec2(0, 0), Font, text);
    bg1helper.Flush();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawBackground(const Sifteo::AssetImage &asset)
{
    EnableBg0SprBg1Video();
    
    Video().BG0_drawAsset(Vec2(0, 0), asset);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawBackgroundWithText(
    const Sifteo::AssetImage &asset,
    const char *text, const Sifteo::Vec2 &textPosition)
{
    ASSERT(text != NULL);
    
    EnableBg0SprBg1Video();
    
    Video().BG0_drawAsset(Vec2(0, 0), asset);
    
    BG1Helper bg1helper(mCube);
    bg1helper.DrawText(textPosition, Font, text);
    bg1helper.Flush();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawCutscene(const char *text)
{
    ASSERT(text != NULL);
    
    EnableBg0SprBg1Video();
    
    Video().BG0_drawAsset(Vec2(0, 0), CutsceneBackground);
    
    Video().setSpriteImage(0, CutsceneSprites, 0);
    Video().setSpriteImage(1, CutsceneSprites, 1);
    
    // TODO: Put super-lame animation code elsewhere
    
    if (!mCutsceneSpriteJump0)
    {
        if (mRandom.randrange(8) == 0)
        {
            mCutsceneSpriteJump0 = true;
        }
    }
    else
    {
        if (mRandom.randrange(1) == 0)
        {
            mCutsceneSpriteJump0 = false;
        }
    }
    
    if (!mCutsceneSpriteJump1)
    {
        if (mRandom.randrange(16) == 0)
        {
            mCutsceneSpriteJump1 = true;
        }
    }
    else
    {
        if (mRandom.randrange(1) == 0)
        {
            mCutsceneSpriteJump1 = false;
        }
    }
    
    Video().moveSprite(0, Vec2( 0, mCutsceneSpriteJump0 ? 80 : 72));
    Video().moveSprite(1, Vec2(64, mCutsceneSpriteJump1 ? 80: 72));
    
    BG1Helper bg1helper(mCube);
    bg1helper.DrawAsset(Vec2(0, 0), CutsceneTextBubble);
    bg1helper.DrawText(Vec2(1, 1), Font, text);
    bg1helper.Flush();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::EnableBg0SprBg1Video()
{
    Video().set();
    Video().clear();
    
    for (int i = 0; i < (NUM_SIDES * 2); ++i)
    {
        Video().hideSprite(i);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::ClearBg1()
{
    BG1Helper bg1helper(mCube);
    bg1helper.Flush();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CubeWrapper::IsLoadingAssets()
{
    ASSERT(IsEnabled());
    
    return mCube.assetDone(GameAssets);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::LoadAssets()
{
    ASSERT(IsEnabled());
    
    mCube.loadAssets(GameAssets);
    
    VidMode_BG0_ROM rom(mCube.vbuf);
    rom.init();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawLoadingAssets()
{
    ASSERT(IsEnabled());
    
    VidMode_BG0_ROM rom(mCube.vbuf);
    rom.BG0_progressBar(Vec2(0, 0), mCube.assetProgress(GameAssets, VidMode_BG0::LCD_width), 16);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CubeWrapper::IsEnabled() const
{
    return mEnabled;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::Enable(Cube::ID cubeId, unsigned int buddyId)
{
    ASSERT(!IsEnabled());
    
    mCube.enable(cubeId);
    
    // This ensure proper video state is set, even if we have kLoadAssets = false.
    Video().setWindow(0, VidMode::LCD_height);
    
    mEnabled = true;
    mBuddyId = buddyId;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::Disable()
{
    ASSERT(IsEnabled());
    
    mEnabled = false;
    mCube.disable();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

const Piece &CubeWrapper::GetPiece(Cube::Side side) const
{
    ASSERT(side >= 0 && side < int(arraysize(mPieceOffsets)));
    
    return mPieces[side];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::SetPiece(Cube::Side side, const Piece &piece)
{
    ASSERT(side >= 0 && side < int(arraysize(mPieceOffsets)));
    
    mPieces[side] = piece;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

const Piece &CubeWrapper::GetPieceSolution(Cube::Side side) const
{
    ASSERT(side >= 0 && side < int(arraysize(mPieceOffsets)));
    
    return mPiecesSolution[side];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::SetPieceSolution(Cube::Side side, const Piece &piece)
{
    ASSERT(side >= 0 && side < int(arraysize(mPieceOffsets)));
    
    mPiecesSolution[side] = piece;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

int CubeWrapper::GetPieceOffset(Cube::Side side) const
{
    ASSERT(side >= 0 && side < int(arraysize(mPieceOffsets)));
    
    return mPieceOffsets[side];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::SetPieceOffset(Cube::Side side, int offset)
{
    ASSERT(side >= 0 && side < int(arraysize(mPieceOffsets)));
    
    mPieceOffsets[side] = offset;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CubeWrapper::IsSolved() const
{
    ASSERT(IsEnabled());
    
    for (unsigned int i = 0; i < arraysize(mPiecesSolution); ++i)
    {
        if (mPiecesSolution[i].mMustSolve)
        {
            if (mPieces[i].mBuddy != mPiecesSolution[i].mBuddy ||
                mPieces[i].mPart != mPiecesSolution[i].mPart)
            {
                return false;
            }
        }
    }
    
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CubeWrapper::IsTouching() const
{
#ifdef SIFTEO_SIMULATOR
    return mCube.touching();
#else
    return false;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Sifteo::VidMode_BG0_SPR_BG1 CubeWrapper::Video()
{
    return VidMode_BG0_SPR_BG1(mCube.vbuf);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawPiece(const Piece &piece, Cube::Side side)
{
    ASSERT(piece.mPart >= 0 && piece.mPart < NUM_SIDES);
    ASSERT(side >= 0 && side < NUM_SIDES);
    
    int spriteLayer0 = side;
    int spriteLayer1 = side + NUM_SIDES;
    
    if (piece.mAttribute == Piece::ATTR_FIXED)
    {
        Video().setSpriteImage(spriteLayer0, BuddyFacePartFixed);
    }
    else
    {
        Video().hideSprite(spriteLayer0);
    }
    
    if (piece.mAttribute == Piece::ATTR_HIDDEN)
    {
        Video().setSpriteImage(spriteLayer1, BuddyFacePartHidden);
    }
    else
    {
        int rotation = side - piece.mPart;
        if (rotation < 0)
        {
            rotation += 4;
        }
        
        const Sifteo::PinnedAssetImage &asset = GetBuddyFacePartsAsset(piece.mBuddy);
        unsigned int frame = (rotation * NUM_SIDES) + piece.mPart;
        
        ASSERT(frame < asset.frames);
        Video().setSpriteImage(spriteLayer1, asset, frame);
    }
    
    Vec2 point = kPartPositions[side];
    
    switch(side)
    {
        case SIDE_TOP:
        {
            point.y += mPieceOffsets[side];
            break;
        }
        case SIDE_LEFT:
        {
            point.x += mPieceOffsets[side];
            break;
        }
        case SIDE_BOTTOM:
        {
            point.y -= mPieceOffsets[side];
            break;
        }
        case SIDE_RIGHT:
        {
            point.x -= mPieceOffsets[side];
            break;
        }
    }
    
    ASSERT(kPieceAnimPeriod > 0.0f);
    float w = 2.0f * M_PI / kPieceAnimPeriod;
    float x = kPieceAnimX * cosf(w * mPieceAnimT);
    float y = kPieceAnimY * sinf(w * mPieceAnimT);
    
    point.x += int(x);
    point.y += int(y);
    
    Video().moveSprite(spriteLayer0, point);
    Video().moveSprite(spriteLayer1, point);
    
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawBanner(const Sifteo::AssetImage &asset)
{
    BG1Helper bg1helper(mCube);
    bg1helper.DrawAsset(Vec2(0, 0), asset);
    bg1helper.Flush();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawScoreBanner(const Sifteo::AssetImage &asset, int minutes, int seconds)
{
    BG1Helper bg1helper(mCube);
    bg1helper.DrawAsset(Vec2(0, 0), asset); // Banner Background
    
    const AssetImage &font = mCube.id() == 0 ? FontScoreBlue : FontScoreOrange;
    
    int x = 11;
    int y = 0;
    bg1helper.DrawAsset(Vec2(x++, y), font, minutes / 10); // Mintues (10s)
    bg1helper.DrawAsset(Vec2(x++, y), font, minutes % 10); // Minutes ( 1s)
    bg1helper.DrawAsset(Vec2(x++, y), font, 10); // ":"
    bg1helper.DrawAsset(Vec2(x++, y), font, seconds / 10); // Seconds (10s)
    bg1helper.DrawAsset(Vec2(x++, y), font, seconds % 10); // Seconds ( 1s)
    
    bg1helper.Flush();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawHintBar(Cube::Side side)
{
    ASSERT(side >= 0 && side < NUM_SIDES);
    
    BG1Helper bg1helper(mCube);
    bg1helper.DrawAsset(GetHintBarPoint(side), GetHintBarAsset(mCube.id(), side));
    bg1helper.Flush();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}
