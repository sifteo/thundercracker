/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "CubeWrapper.h"
#include <sifteo/string.h>
#include "Config.h"
#include "assets.gen.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

using namespace Sifteo;

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace Buddies { namespace {

///////////////////////////////////////////////////////////////////////////////////////////////////
// Buddy Assets
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef BUDDY_PIECES_USE_SPRITES
typedef PinnedAssetImage BuddyPartAssetImage;
#else
typedef AssetImage BuddyPartAssetImage;
#endif

const AssetImage *kBuddyBackgrounds[] =
{
    &BuddyBackground0,
    &BuddyBackground1,
    &BuddyBackground2,
    &BuddyBackground3,
    &BuddyBackground4,
    &BuddyBackground5,
    &BuddyBackground6,
    &BuddyBackground7,
    &BuddyBackground8,
    &BuddyBackground9,
    &BuddyBackground10,
    &BuddyBackground11,
    &UiFaceHolder,
};

const BuddyPartAssetImage *kBuddyParts[] =
{
    &BuddyParts0,
    &BuddyParts1,
    &BuddyParts2,
    &BuddyParts3,
    &BuddyParts4,
    &BuddyParts5,
    &BuddyParts6,
    &BuddyParts7,
    &BuddyParts8,
    &BuddyParts9,
    &BuddyParts10,
    &BuddyParts11,
    NULL,
};

#ifdef BUDDY_PIECES_USE_SPRITES

const Int2 kPartPositions[NUM_SIDES] =
{
    vec(32, -8),
    vec(-8, 32),
    vec(32, 72),
    vec(72, 32),
};

#else

const Int2 kPartPositions[NUM_SIDES] =
{
    vec(40,  0),
    vec( 0, 40),
    vec(40, 80),
    vec(80, 40),
};

const BG1Mask kPartsMask =
    BG1Mask::filled(vec( 5,  0), vec(6, 6)) |
    BG1Mask::filled(vec( 0,  5), vec(6, 6)) |
    BG1Mask::filled(vec( 5, 10), vec(6, 6)) |
    BG1Mask::filled(vec(10,  5), vec(6, 6));

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void GetTextSize(const char *text, int &numLines, int &maxLength)
{
    ASSERT(text != NULL);
    
    numLines = 0;
    maxLength = 0;
    
    String<128> buffer;
    buffer << text;
    
    int length = 0;
    
    for (int i = 0; i < buffer.size(); ++i)
    {
        if (buffer[i] == '\n')
        {
            if (length > maxLength)
            {
                maxLength = length;
            }
            length = 0;
            ++numLines;
        }
        else
        {
            ++length;
        }
    }
    
    if (length > maxLength)
    {
        maxLength = length;
    }
    length = 0;
    ++numLines;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

CubeWrapper::CubeWrapper()
    : mVideoBuffer()
    , mCubeId(0)
    , mBg1Mask(BG1Mask::empty())
    , mEnabled(false)
    , mBuddyId(BUDDY_GLUV)
    , mPieces()
    , mPiecesSolution()
    , mPieceOffsets()
    , mPieceBlinking(NO_SIDE)
    , mPieceBlinkTimer(0.0f)
    , mPieceBlinkingOn(false)
    , mBumpTimer(0.0f)
    , mBumpSide(TOP)
{
    for (unsigned int i = 0; i < NUM_SIDES; ++i)
    {
        mPieceOffsets[i] = vec(0, 0);
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
        mPieceOffsets[i] = vec(0, 0);
    }
    mPieceBlinking = NO_SIDE;
    mPieceBlinkTimer = 0.0f;
    mPieceBlinkingOn = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CubeWrapper::Update(float dt)
{
    bool blinked = false;
    
    if (mPieceBlinking >= 0 && mPieceBlinking < NUM_SIDES)
    {
        ASSERT(mPieceBlinkTimer > 0.0f);
        
        mPieceBlinkTimer -= dt;
        if (mPieceBlinkTimer <= 0.0f)
        {
            mPieceBlinkingOn = !mPieceBlinkingOn;
            mPieceBlinkTimer += kHintBlinkTimerDuration;
            
            blinked = !mPieceBlinkingOn;
        }
    }
    
    if (mBumpTimer > 0.0f)
    {
        mBumpTimer -= dt;
        if (mBumpTimer <= 0.0f)
        {
            mBumpTimer = 0.0f;
        }
    }
    
    return blinked;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawClear()
{
    mVideoBuffer.initMode(BG0_SPR_BG1);
    mVideoBuffer.bg0.erase();
    mVideoBuffer.bg1.erase();
    mVideoBuffer.sprites.erase();
    mBg1Mask.clear();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawBuddy()
{
    ASSERT(IsEnabled());
    
    ASSERT(mBuddyId < arraysize(kBuddyBackgrounds));
    if (const AssetImage *asset = kBuddyBackgrounds[mBuddyId])
    {
        // Parallax Shift
        Byte2 offset = GetTiltState();
        offset.x *= -kParallaxDistance;
        offset.y *= -kParallaxDistance;
        
        // Bump
        if (mBumpTimer > 0.0f)
        {
            float t = mBumpTimer / kBumpTimerDuration;
            int d = float(kBumpDistance) * t;
            
            switch (mBumpSide)
            {
                case TOP:
                    offset.y += d;
                    break;
                case LEFT:
                    offset.x += d;
                    break;
                case BOTTOM:
                    offset.y -= d;
                    break;
                case RIGHT:
                    offset.x -= d;
                    break;
                default:
                    break;
            }
        }
        
        ScrollBackground(vec(int(TILE), int(TILE)) + offset);
        
        // Draw the actual asset
        DrawBackground(*asset);
    }
    
    mVideoBuffer.bg1.setMask(kPartsMask);
    
    for (int i = 0; i < NUM_SIDES; ++i)
    {
        if (mPieceBlinking != i || !mPieceBlinkingOn)
        {
            DrawPiece(mPieces[i], Side(i));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawBackground(const AssetImage &asset)
{
    mVideoBuffer.bg0.image(vec(0, 0), asset);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawBackgroundPartial(
    Sifteo::Int2 position,
    Sifteo::Int2 offset,
    Sifteo::Int2 size,
    const Sifteo::AssetImage &asset)
{
    mVideoBuffer.bg0.image(position, size, asset, offset);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::ScrollBackground(Int2 position)
{
    mVideoBuffer.bg0.setPanning(position);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawSprite(
    int spriteIndex,
    Int2 position,
    const Sifteo::PinnedAssetImage &asset, unsigned int assetFrame)
{
    ASSERT(spriteIndex >= 0 && spriteIndex < _SYS_VRAM_SPRITES);
    mVideoBuffer.sprites[spriteIndex].setImage(asset, assetFrame);
    mVideoBuffer.sprites[spriteIndex].move(position);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawUiAsset(
    Int2 position,
    const AssetImage &asset, unsigned int assetFrame)
{
    mBg1Mask.fill(position, asset.tileSize());
    mVideoBuffer.bg1.setMask(mBg1Mask);
    mVideoBuffer.bg1.image(position, asset, assetFrame);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawUiAssetPartial(
    Sifteo::Int2 position,
    Sifteo::Int2 offset,
    Sifteo::Int2 size,
    const Sifteo::AssetImage &asset, unsigned int assetFrame)
{
    mBg1Mask.fill(position, size);
    mVideoBuffer.bg1.setMask(mBg1Mask);
    mVideoBuffer.bg1.image(position, size, asset, offset, assetFrame);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::DrawUiText(
    Int2 position,
    const AssetImage &assetFont,
    const char *text)
{
    ASSERT(text != NULL);
    
    int numLines = 0;
    int maxLength = 0;
    GetTextSize(text, numLines, maxLength);
    
    mBg1Mask.fill(position, vec(maxLength, 2 * numLines));
    mVideoBuffer.bg1.setMask(mBg1Mask);
    mVideoBuffer.bg1.text(position, assetFont, text);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::ScrollUi(Int2 position)
{
    mVideoBuffer.bg1.setPanning(position);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PCubeID CubeWrapper::GetId()
{
    return mCubeId;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CubeWrapper::IsEnabled() const
{
    return mEnabled;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::Enable(PCubeID cubeId)
{
    ASSERT(!IsEnabled());
    
    mCubeId = cubeId;
    mVideoBuffer.initMode(BG0_ROM);
    mVideoBuffer.attach(mCubeId);
    
    // This ensure proper video state is set, even if we have kLoadAssets = false.
    mVideoBuffer.setWindow(0, LCD_height);
    
    mEnabled = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::Disable()
{
    ASSERT(IsEnabled());
    
    mEnabled = false;
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

const Piece &CubeWrapper::GetPiece(Side side) const
{
    ASSERT(side >= 0 && side < int(arraysize(mPieceOffsets)));
    
    return mPieces[side];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::SetPiece(Side side, const Piece &piece)
{
    ASSERT(side >= 0 && side < int(arraysize(mPieceOffsets)));
    
    mPieces[side] = piece;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

const Piece &CubeWrapper::GetPieceSolution(Side side) const
{
    ASSERT(side >= 0 && side < int(arraysize(mPieceOffsets)));
    
    return mPiecesSolution[side];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::SetPieceSolution(Side side, const Piece &piece)
{
    ASSERT(side >= 0 && side < int(arraysize(mPieceOffsets)));
    
    mPiecesSolution[side] = piece;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Int2 CubeWrapper::GetPieceOffset(Side side) const
{
    ASSERT(side >= 0 && side < int(arraysize(mPieceOffsets)));
    
    return mPieceOffsets[side];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::SetPieceOffset(Side side, Int2 offset)
{
    ASSERT(side >= 0 && side < int(arraysize(mPieceOffsets)));
    
    mPieceOffsets[side] = offset;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::StartPieceBlinking(Side side)
{
    mPieceBlinking = side;
    mPieceBlinkTimer = kHintBlinkTimerDuration;
    mPieceBlinkingOn = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::StopPieceBlinking()
{
    mPieceBlinking = NO_SIDE;
    mPieceBlinkTimer = 0.0f;
    mPieceBlinkingOn = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Byte2 CubeWrapper::GetTiltState() const
{
    return mCubeId.tilt();
}
 
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Byte3 CubeWrapper::GetAccelState() const
{
    return mCubeId.accel();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CubeWrapper::StartBump(Side side)
{
    mBumpTimer = kBumpTimerDuration;
    mBumpSide = side;
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
    return mCubeId.isTouching();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef BUDDY_PIECES_USE_SPRITES

void CubeWrapper::DrawPiece(const Piece &piece, Cube::Side side)
{
    unsigned int spriteOver = side + 0;
    unsigned int spriteUnder = side + NUM_SIDES;
    
    // TODO: nudge support
    Int2 accelState = GetAccelState();
    float x = float(accelState.x + 61) / (123.0f * 0.5f) - 1.0f;
    float y = float(accelState.y + 61) / (123.0f * 0.5f) - 1.0f;
    float d = 8.0f;
    Int2 nudge = vec(x * d, y * d);
    
    Int2 point = kPartPositions[side] + nudge;
    switch(side)
    {
        case TOP:
        {
            point.x += mPieceOffsets[side].x;
            point.y += mPieceOffsets[side].y;
            break;
        }
        case LEFT:
        {
            point.x += mPieceOffsets[side].x;
            point.y += mPieceOffsets[side].y;
            break;
        }
        case BOTTOM:
        {
            point.x -= mPieceOffsets[side].x;
            point.y -= mPieceOffsets[side].y;
            break;
        }
        case RIGHT:
        {
            point.x -= mPieceOffsets[side].x;
            point.y += mPieceOffsets[side].y;
            break;
        }
    }
    
    ASSERT(piece.GetBuddy() < arraysize(kBuddyParts));
    if (const PinnedAssetImage *asset = kBuddyParts[piece.GetBuddy()])
    {
        unsigned int frame = (piece.GetRotation() * NUM_SIDES) + piece.GetPart();
        ASSERT(frame < asset->frames);
        mVideoBuffer.setSpriteImage(spriteUnder, *asset, frame);
        mVideoBuffer.moveSprite(spriteUnder, point);
    }
    
    if (piece.GetAttribute() == Piece::ATTR_FIXED)
    {
        mVideoBuffer.setSpriteImage(spriteOver, BuddyPartFixed, 0);
        mVideoBuffer.moveSprite(spriteOver, point);
    }
    else if (piece.GetAttribute() == Piece::ATTR_HIDDEN)
    {
        mVideoBuffer.setSpriteImage(spriteOver, BuddyPartHidden, 0);
        mVideoBuffer.moveSprite(spriteOver, point);
        mVideoBuffer.hideSprite(spriteUnder);
    }
}

#else

void CubeWrapper::DrawPiece(const Piece &piece, Side side)
{
    ASSERT(piece.GetBuddy() < arraysize(kBuddyParts));
    if (const AssetImage *assetImage = kBuddyParts[piece.GetBuddy()])
    {
        AssetImage asset = *assetImage;
        unsigned int frame = (piece.GetRotation() * NUM_SIDES) + piece.GetPart();
        ASSERT(frame < asset.numFrames());
        
        if (piece.GetAttribute() == Piece::ATTR_HIDDEN)
        {
            asset = BuddyPartHidden;
            frame = 0;
        }
        
        // Nudge
        Byte2 tiltState = GetTiltState();
        Int2 nudge = vec(tiltState.x * TILE, tiltState.y * TILE);
        Int2 point = kPartPositions[side] + nudge;
        
        switch(side)
        {
            case TOP:
            {
                point.x += mPieceOffsets[side].x;
                point.y += mPieceOffsets[side].y;
                break;
            }
            case LEFT:
            {
                point.x += mPieceOffsets[side].x;
                point.y += mPieceOffsets[side].y;
                break;
            }
            case BOTTOM:
            {
                point.x -= mPieceOffsets[side].x;
                point.y -= mPieceOffsets[side].y;
                break;
            }
            case RIGHT:
            {
                point.x -= mPieceOffsets[side].x;
                point.y += mPieceOffsets[side].y;
                break;
            }
            default:
            {
                break;
            }
        }
        
        point.x /= float(TILE);
        point.y /= float(TILE);
        
        const int width = asset.tileWidth();
        const int height = asset.tileHeight();
        const int max_tiles_x = LCD_width / TILE;
        const int max_tiles_y = LCD_height / TILE;
        
        // Clamp X
        if (point.x < -width)
        {
            point.x = -width;
        }
        else if (point.x > (max_tiles_x + width))
        {
            point.x = max_tiles_x + width;
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
            
            mVideoBuffer.bg1.image(
                vec(0, point.y),
                vec(width - tiles_off, height),
                asset,
                vec(tiles_off, 0),
                frame);
        }
        else if (point.x < max_tiles_x && (point.x + width) > max_tiles_x)
        {
            int tiles_off = (point.x + width) - max_tiles_x;
            
            mVideoBuffer.bg1.image(
                vec(point.x, point.y),
                vec(width - tiles_off, height),
                asset,
                vec(0, 0),
                frame);
        }
        else if (point.y > -height && point.y < 0)
        {
            int tiles_off = -point.y;
            
            mVideoBuffer.bg1.image(
                vec(point.x, 0),
                vec(width, height - tiles_off),
                asset,
                vec(0, tiles_off),
                frame);
        }
        else if (point.y < max_tiles_y && (point.y + height) > max_tiles_y)
        {
            int tiles_off = (point.y + height) - max_tiles_y;
            
            mVideoBuffer.bg1.image(
                vec(point.x, point.y),
                vec(width, height - tiles_off),
                asset,
                vec(0, 0),
                frame);
        }    
        else if (point.x >= 0 && point.x < max_tiles_x && point.y >= 0 && point.y < max_tiles_y)
        {
            mVideoBuffer.bg1.image(vec(point.x, point.y), asset, frame);
        }
    }
}

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}
