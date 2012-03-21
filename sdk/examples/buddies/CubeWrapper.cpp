/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "CubeWrapper.h"
#include <sifteo/asset.h>
#include "Config.h"
#include "assets.gen.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

using namespace Sifteo;

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace Buddies {

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

const AssetImage &GetBuddyFaceBackgroundAsset(int buddyId)
{
    switch (buddyId)
    {
        default:
        case 0: return BuddyBackground0;
        case 1: return BuddyBackground1;
        case 2: return BuddyBackground2;
        case 3: return BuddyBackground3;
        case 4: return BuddyBackground4;
        case 5: return BuddyBackground5;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef BUDDY_PIECES_USE_SPRITES

const PinnedAssetImage &GetBuddyFacePartsAsset(int buddyId)
{
    switch (buddyId)
    {
        default:
        case 0: return BuddyParts0;
        case 1: return BuddyParts1;
        case 2: return BuddyParts2;
        case 3: return BuddyParts3;
        case 4: return BuddyParts4;
        case 5: return BuddyParts5;
    }
}

const Vec2 kPartPositions[NUM_SIDES] =
{
    Vec2(32, -8),
    Vec2(-8, 32),
    Vec2(32, 72),
    Vec2(72, 32),
};

#else

const AssetImage &GetBuddyFacePartsAsset(int buddyId)
{
    switch (buddyId)
    {
        default:
        case 0: return BuddyParts0;
        case 1: return BuddyParts1;
        case 2: return BuddyParts2;
        case 3: return BuddyParts3;
        case 4: return BuddyParts4;
        case 5: return BuddyParts5;
    }
}

const Vec2 kPartPositions[NUM_SIDES] =
{
    Vec2(40,  0),
    Vec2( 0, 40),
    Vec2(40, 80),
    Vec2(80, 40),
};

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

CubeWrapper::CubeWrapper()
    : mCube()
    , mBg1Helper(mCube)
    , mEnabled(false)
    , mBuddyId(0)
    , mPieces()
    , mPiecesSolution()
    , mPieceOffsets()
    , mPieceBlinking(SIDE_UNDEFINED)
    , mPieceBlinkTimer(0.0f)
    , mPieceBlinkingOn(false)
    , mCutsceneSpriteJumpRandom()
    , mCutsceneSpriteJump0(false)
    , mCutsceneSpriteJump1(false)
{
    for (unsigned int i = 0; i < NUM_SIDES; ++i)
    {
        mPieceOffsets[i] = Vec2(0, 0);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::Reset()
{
    for (unsigned int i = 0; i < NUM_SIDES; ++i)
    {
        mPieces[i] = Piece();
        mPiecesSolution[i] = Piece();
        mPieceOffsets[i] = Vec2(0, 0);
    }
    mPieceBlinking = SIDE_UNDEFINED;
    mPieceBlinkTimer = 0.0f;
    mPieceBlinkingOn = false;
    mCutsceneSpriteJump0 = false;
    mCutsceneSpriteJump1 = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::Update(float dt)
{
    if (mPieceBlinking >= 0 && mPieceBlinking < NUM_SIDES)
    {
        ASSERT(mPieceBlinkTimer > 0.0f);
        
        mPieceBlinkTimer -= dt;
        if (mPieceBlinkTimer <= 0.0f)
        {
            mPieceBlinkingOn = !mPieceBlinkingOn;
            mPieceBlinkTimer += kHintBlinkTimerDuration;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawClear()
{
    Video().set();
    Video().clear();
    Video().BG0_setPanning(Vec2(0, 0));
    Video().BG1_setPanning(Vec2(0, 0));
    
    for (int i = 0; i < _SYS_VRAM_SPRITES; ++i)
    {
        Video().hideSprite(i);
    }
    
    mBg1Helper.Clear();
    mBg1Helper.Flush();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawFlush()
{
    mBg1Helper.Flush();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CubeWrapper::DrawNeedsSync()
{
    return mBg1Helper.NeedFinish();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawBuddy()
{
    ASSERT(IsEnabled());
    
    DrawBackground(GetBuddyFaceBackgroundAsset(mBuddyId));
    
    for (unsigned int i = 0; i < NUM_SIDES; ++i)
    {
        if (mPieceBlinking != int(i) || !mPieceBlinkingOn)
        {
            DrawPiece(mPieces[i], i);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawBackground(const AssetImage &asset)
{
    Video().BG0_drawAsset(Vec2(0, 0), asset);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawBackgroundPartial(
    const Sifteo::Vec2 &position,
    const Sifteo::Vec2 &offset,
    const Sifteo::Vec2 &size,
    const Sifteo::AssetImage &asset)
{
    Video().BG0_drawPartialAsset(position, offset, size, asset);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::ScrollBackground(const Vec2 &position)
{
    Video().BG0_setPanning(position);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawSprite(
    int spriteIndex,
    Vec2 position,
    const Sifteo::PinnedAssetImage &asset, unsigned int assetFrame)
{
    ASSERT(spriteIndex >= 0 && spriteIndex < _SYS_VRAM_SPRITES);
    Video().setSpriteImage(spriteIndex, asset, assetFrame);
    Video().moveSprite(spriteIndex, position);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawUiAsset(
    const Vec2 &position,
    const AssetImage &asset, unsigned int assetFrame)
{
    mBg1Helper.DrawAsset(position, asset, assetFrame);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawUiAssetPartial(
    const Sifteo::Vec2 &position,
    const Sifteo::Vec2 &offset,
    const Sifteo::Vec2 &size,
    const Sifteo::AssetImage &asset, unsigned int assetFrame)
{
    mBg1Helper.DrawPartialAsset(position, offset, size, asset, assetFrame);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawUiText(
    const Vec2 &position,
    const AssetImage &assetFont,
    const char *text)
{
    ASSERT(text != NULL);
    mBg1Helper.DrawText(position, assetFont, text);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::ScrollUi(const Vec2 &position)
{
    Video().BG1_setPanning(position);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::UpdateCutscene(int jumpChanceA, int jumpChanceB)
{
    UpdateCutsceneSpriteJump(mCutsceneSpriteJump0, jumpChanceA, 1);
    UpdateCutsceneSpriteJump(mCutsceneSpriteJump1, jumpChanceB, 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawCutsceneShuffle(const Sifteo::Vec2 &scroll)
{
    ASSERT(1 <= _SYS_VRAM_SPRITES);
    const unsigned int maxTilesX = VidMode::LCD_width / VidMode::TILE;
    const unsigned int maxTilesY = VidMode::LCD_width / VidMode::TILE;
            
    Video().BG0_drawPartialAsset(
        Vec2(0, 0),
        Vec2(-scroll.x, 0),
        Vec2(maxTilesX + scroll.x, maxTilesY),
        UiCongratulations);
    
    switch (GetId())
    {
        default:
        case 0:
            Video().setSpriteImage(0, BuddySpriteFrontZorg, 0);
            break;
        case 1:
            Video().setSpriteImage(0, BuddySpriteFrontRike, 0);
            break;
        case 2:
            Video().setSpriteImage(0, BuddySpriteFrontGluv, 0);
            break;
    }
    
    int jump_offset = 8;
    
    Video().moveSprite(
        0,
        Vec2(
            (VidMode::LCD_width / 2) - 32 + (scroll.x * VidMode::TILE),
            mCutsceneSpriteJump0 ?
                VidMode::LCD_height / 2 - 32 :
                VidMode::LCD_height / 2 - 32 + jump_offset));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawCutsceneStory(const char *text)
{
    ASSERT(text != NULL);
    ASSERT(2 <= _SYS_VRAM_SPRITES);
    
    Video().setSpriteImage(0, BuddySpriteCutsceneGluv, 0);
    Video().setSpriteImage(1, BuddySpriteCutsceneRike, 0);
    
    Video().moveSprite(0, Vec2( 0, mCutsceneSpriteJump0 ? 60 : 66));
    Video().moveSprite(1, Vec2(64, mCutsceneSpriteJump1 ? 60 : 66));
    
    if (text[0] == '<')
    {
        Video().BG0_drawAsset(Vec2(0, 0), StoryCutsceneBackgroundLeft);
        mBg1Helper.DrawText(Vec2(1, 1), UiFontBlack, text + 1);
    }
    else if (text[0] == '>')
    {
        Video().BG0_drawAsset(Vec2(0, 0), StoryCutsceneBackgroundRight);
        mBg1Helper.DrawText(Vec2(1, 1), UiFontBlack, text + 1);
    }
    else
    {
        Video().BG0_drawAsset(Vec2(0, 0), StoryCutsceneBackgroundLeft);
        mBg1Helper.DrawText(Vec2(1, 1), UiFontBlack, text);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawUnlocked3Sprite(const Sifteo::Vec2 &scroll)
{
    ASSERT(1 <= _SYS_VRAM_SPRITES);
    Video().setSpriteImage(0, BuddySpriteFrontGluv, 0);
    
    int jump_offset = 4;
    
    int x = (VidMode::LCD_width / 2) - 32 + (scroll.x * VidMode::TILE);
    int y = mCutsceneSpriteJump0 ? 28 - jump_offset : 28;
    y += -VidMode::LCD_height + ((scroll.y + 2) * VidMode::TILE); // TODO: +2 is fudge, refactor
    
    Video().moveSprite(0, Vec2(x, y));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawUnlocked4Sprite(const Sifteo::Vec2 &scroll)
{
    ASSERT(1 <= _SYS_VRAM_SPRITES);
    Video().setSpriteImage(0, BuddySpriteFrontGluv, 0);
    Video().moveSprite(0, Vec2((VidMode::LCD_width / 2) - 32 + (scroll.x * VidMode::TILE), 28));
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

Cube::ID CubeWrapper::GetId()
{
    return mCube.id();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CubeWrapper::IsEnabled() const
{
    return mEnabled;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::Enable(Cube::ID cubeId)
{
    ASSERT(!IsEnabled());
    
    mCube.enable(cubeId);
    
    // This ensure proper video state is set, even if we have kLoadAssets = false.
    Video().setWindow(0, VidMode::LCD_height);
    
    mEnabled = true;
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

unsigned int CubeWrapper::GetBuddyId() const
{
    return mBuddyId;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::SetBuddyId(unsigned int buddyId)
{
    mBuddyId = buddyId;
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

const Vec2 &CubeWrapper::GetPieceOffset(Cube::Side side) const
{
    ASSERT(side >= 0 && side < int(arraysize(mPieceOffsets)));
    
    return mPieceOffsets[side];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::SetPieceOffset(Cube::Side side, const Vec2 &offset)
{
    ASSERT(side >= 0 && side < int(arraysize(mPieceOffsets)));
    
    mPieceOffsets[side] = offset;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::StartPieceBlinking(Cube::Side side)
{
    mPieceBlinking = side;
    mPieceBlinkTimer = kHintBlinkTimerDuration;
    mPieceBlinkingOn = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::StopPieceBlinking()
{
    mPieceBlinking = SIDE_UNDEFINED;
    mPieceBlinkTimer = 0.0f;
    mPieceBlinkingOn = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Cube::TiltState CubeWrapper::GetTiltState() const
{
    return mCube.getTiltState();
}
 
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Vec2 CubeWrapper::GetAccelState() const
{
    return mCube.physicalAccel();
}
 
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CubeWrapper::IsSolved() const
{
    ASSERT(IsEnabled());
    
    for (unsigned int i = 0; i < arraysize(mPiecesSolution); ++i)
    {
        if (mPiecesSolution[i].GetMustSolve())
        {
            if (mPieces[i].GetBuddy() != mPiecesSolution[i].GetBuddy() ||
                mPieces[i].GetPart() != mPiecesSolution[i].GetPart())
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

VidMode_BG0_SPR_BG1 CubeWrapper::Video()
{
    return VidMode_BG0_SPR_BG1(mCube.vbuf);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef BUDDY_PIECES_USE_SPRITES

void CubeWrapper::DrawPiece(const Piece &piece, Cube::Side side)
{
    const PinnedAssetImage &asset = GetBuddyFacePartsAsset(piece.GetBuddy());
    
    unsigned int frame = (piece.GetRotation() * NUM_SIDES) + piece.GetPart();
    ASSERT(frame < asset.frames);
    
    unsigned int spriteOver = side + 0;
    unsigned int spriteUnder = side + NUM_SIDES;
    
    Video().setSpriteImage(spriteUnder, asset, frame);
    
    Vec2 point = kPartPositions[side];
    switch(side)
    {
        case SIDE_TOP:
        {
            point.x += mPieceOffsets[side].x;
            point.y += mPieceOffsets[side].y;
            break;
        }
        case SIDE_LEFT:
        {
            point.x += mPieceOffsets[side].x;
            point.y += mPieceOffsets[side].y;
            break;
        }
        case SIDE_BOTTOM:
        {
            point.x -= mPieceOffsets[side].x;
            point.y -= mPieceOffsets[side].y;
            break;
        }
        case SIDE_RIGHT:
        {
            point.x -= mPieceOffsets[side].x;
            point.y += mPieceOffsets[side].y;
            break;
        }
    }
    Video().moveSprite(spriteUnder, point);
    
    if (piece.GetAttribute() == Piece::ATTR_FIXED)
    {
        Video().setSpriteImage(spriteOver, BuddyPartFixed, 0);
        Video().moveSprite(spriteOver, point);
    }
    else if (piece.GetAttribute() == Piece::ATTR_HIDDEN)
    {
        Video().setSpriteImage(spriteOver, BuddyPartHidden, 0);
        Video().moveSprite(spriteOver, point);
        Video().hideSprite(spriteUnder);
    }
}

#else

void CubeWrapper::DrawPiece(const Piece &piece, Cube::Side side)
{
    AssetImage asset = GetBuddyFacePartsAsset(piece.GetBuddy());
    unsigned int frame = (piece.GetRotation() * NUM_SIDES) + piece.GetPart();
    ASSERT(frame < asset.frames);
    
    if (piece.GetAttribute() == Piece::ATTR_HIDDEN)
    {
        asset = BuddyPartHidden;
        frame = 0;
    }
    
    Vec2 point = kPartPositions[side];
    
    switch(side)
    {
        case SIDE_TOP:
        {
            point.x += mPieceOffsets[side].x;
            point.y += mPieceOffsets[side].y;
            break;
        }
        case SIDE_LEFT:
        {
            point.x += mPieceOffsets[side].x;
            point.y += mPieceOffsets[side].y;
            break;
        }
        case SIDE_BOTTOM:
        {
            point.x -= mPieceOffsets[side].x;
            point.y -= mPieceOffsets[side].y;
            break;
        }
        case SIDE_RIGHT:
        {
            point.x -= mPieceOffsets[side].x;
            point.y += mPieceOffsets[side].y;
            break;
        }
    }
    
    point.x /= float(VidMode::TILE);
    point.y /= float(VidMode::TILE);
    
    const int width = asset.width;
    const int height = asset.height;
    const int max_tiles_x = VidMode::LCD_width / VidMode::TILE;
    const int max_tiles_y = VidMode::LCD_height / VidMode::TILE;
    
    // Clamp X
    if (point.x < -width)
    {
        point.x = -width;
    }
    else if (point.x > (max_tiles_x + width))
    {
        point.x = max_tiles_x + asset.width;
    }
    
    // Clamp Y
    if (point.y < -height)
    {
        point.y = -height;
    }
    else if (point.y > (max_tiles_y + height))
    {
        point.y = max_tiles_y + height;
    }
    
    // Draw partial or full asset
    if (point.x > -width && point.x < 0)
    {
        int tiles_off = -point.x;
        
        mBg1Helper.DrawPartialAsset(
            Vec2(0, point.y),
            Vec2(tiles_off, 0),
            Vec2(width - tiles_off, height),
            asset, frame);
    }
    else if (point.x < max_tiles_x && (point.x + width) > max_tiles_x)
    {
        int tiles_off = (point.x + width) - max_tiles_x;
        
        mBg1Helper.DrawPartialAsset(
            Vec2(point.x, point.y),
            Vec2(0, 0),
            Vec2(width - tiles_off, height),
            asset, frame);
    }
    else if (point.y > -height && point.y < 0)
    {
        int tiles_off = -point.y;
        
        mBg1Helper.DrawPartialAsset(
            Vec2(point.x, 0),
            Vec2(0, tiles_off),
            Vec2(width, height - tiles_off),
            asset, frame);
    }
    else if (point.y < max_tiles_y && (point.y + height) > max_tiles_y)
    {
        int tiles_off = (point.y + height) - max_tiles_y;
        
        mBg1Helper.DrawPartialAsset(
            Vec2(point.x, point.y),
            Vec2(0, 0),
            Vec2(width, height - tiles_off),
            asset, frame);
    }    
    else if (point.x >= 0 && point.x < max_tiles_x && point.y >= 0 && point.y < max_tiles_y)
    {
        mBg1Helper.DrawAsset(Vec2(point.x, point.y), asset, frame);
    }
}

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::UpdateCutsceneSpriteJump(bool &cutsceneSpriteJump, int upChance, int downChance)
{
    if (!cutsceneSpriteJump)
    {
        if (mCutsceneSpriteJumpRandom.randrange(upChance) == 0)
        {
            cutsceneSpriteJump = true;
        }
    }
    else
    {
        if (mCutsceneSpriteJumpRandom.randrange(downChance) == 0)
        {
            cutsceneSpriteJump = false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}
