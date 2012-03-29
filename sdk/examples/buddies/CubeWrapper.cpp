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

const AssetImage *GetBuddyFaceBackgroundAsset(int buddyId)
{
    switch (buddyId)
    {
        default:
        case BUDDY_GLUV: return &BuddyBackground0;
        case BUDDY_SULI: return &BuddyBackground1;
        case BUDDY_RIKE: return &BuddyBackground2;
        case BUDDY_BOFF: return &BuddyBackground3;
        case BUDDY_ZORG: return &BuddyBackground4;
        case BUDDY_MARO: return &BuddyBackground5;
        case BUDDY_INVISIBLE: return &UiFaceHolder;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef BUDDY_PIECES_USE_SPRITES

const PinnedAssetImage *GetBuddyFacePartsAsset(int buddyId)
{
    switch (buddyId)
    {
        default:
        case BUDDY_GLUV: return &BuddyParts0;
        case BUDDY_SULI: return &BuddyParts1;
        case BUDDY_RIKE: return &BuddyParts2;
        case BUDDY_BOFF: return &BuddyParts3;
        case BUDDY_ZORG: return &BuddyParts4;
        case BUDDY_MARO: return &BuddyParts5;
        case BUDDY_INVISIBLE: return NULL;
    }
}

const Int2 kPartPositions[NUM_SIDES] =
{
    Vec2(32, -8),
    Vec2(-8, 32),
    Vec2(32, 72),
    Vec2(72, 32),
};

#else

const AssetImage *GetBuddyFacePartsAsset(int buddyId)
{
    switch (buddyId)
    {
        default:
        case BUDDY_GLUV: return &BuddyParts0;
        case BUDDY_SULI: return &BuddyParts1;
        case BUDDY_RIKE: return &BuddyParts2;
        case BUDDY_BOFF: return &BuddyParts3;
        case BUDDY_ZORG: return &BuddyParts4;
        case BUDDY_MARO: return &BuddyParts5;
        case BUDDY_INVISIBLE: return NULL;
    }
}

const Int2 kPartPositions[NUM_SIDES] =
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
    , mBuddyId(BUDDY_GLUV)
    , mPieces()
    , mPiecesSolution()
    , mPieceOffsets()
    , mPieceBlinking(SIDE_UNDEFINED)
    , mPieceBlinkTimer(0.0f)
    , mPieceBlinkingOn(false)
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
    
    if (const AssetImage *asset = GetBuddyFaceBackgroundAsset(mBuddyId))
    {
        DrawBackground(*asset);
    }
    
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
    Sifteo::Int2 position,
    Sifteo::Int2 offset,
    Sifteo::Int2 size,
    const Sifteo::AssetImage &asset)
{
    Video().BG0_drawPartialAsset(position, offset, size, asset);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::ScrollBackground(Int2 position)
{
    Video().BG0_setPanning(position);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawSprite(
    int spriteIndex,
    Int2 position,
    const Sifteo::PinnedAssetImage &asset, unsigned int assetFrame)
{
    ASSERT(spriteIndex >= 0 && spriteIndex < _SYS_VRAM_SPRITES);
    Video().setSpriteImage(spriteIndex, asset, assetFrame);
    Video().moveSprite(spriteIndex, position);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawUiAsset(
    Int2 position,
    const AssetImage &asset, unsigned int assetFrame)
{
    mBg1Helper.DrawAsset(position, asset, assetFrame);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawUiAssetPartial(
    Sifteo::Int2 position,
    Sifteo::Int2 offset,
    Sifteo::Int2 size,
    const Sifteo::AssetImage &asset, unsigned int assetFrame)
{
    mBg1Helper.DrawPartialAsset(position, offset, size, asset, assetFrame);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawUiText(
    Int2 position,
    const AssetImage &assetFont,
    const char *text)
{
    ASSERT(text != NULL);
    mBg1Helper.DrawText(position, assetFont, text);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::ScrollUi(Int2 position)
{
    Video().BG1_setPanning(position);
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

BuddyId CubeWrapper::GetBuddyId() const
{
    return mBuddyId;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::SetBuddyId(BuddyId buddyId)
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

Int2 CubeWrapper::GetPieceOffset(Cube::Side side) const
{
    ASSERT(side >= 0 && side < int(arraysize(mPieceOffsets)));
    
    return mPieceOffsets[side];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::SetPieceOffset(Cube::Side side, Int2 offset)
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

Int2 CubeWrapper::GetAccelState() const
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
    return mCube.touching();
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
    // TODO: Update this to handle pointer returns

    const PinnedAssetImage &asset = GetBuddyFacePartsAsset(piece.GetBuddy());
    
    unsigned int frame = (piece.GetRotation() * NUM_SIDES) + piece.GetPart();
    ASSERT(frame < asset.frames);
    
    unsigned int spriteOver = side + 0;
    unsigned int spriteUnder = side + NUM_SIDES;
    
    Video().setSpriteImage(spriteUnder, asset, frame);
    
    Int2 point = kPartPositions[side];
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
    if (const AssetImage *assetImage = GetBuddyFacePartsAsset(piece.GetBuddy()))
    {
        AssetImage asset = *assetImage;
        unsigned int frame = (piece.GetRotation() * NUM_SIDES) + piece.GetPart();
        ASSERT(frame < asset.frames);
        
        if (piece.GetAttribute() == Piece::ATTR_HIDDEN)
        {
            asset = BuddyPartHidden;
            frame = 0;
        }
        
        Int2 point = kPartPositions[side];
        
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
}

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}
