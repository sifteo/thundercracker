/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "App.h"
#include <sifteo.h>
#include "Config.h"
#include "assets.gen.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace Buddies { namespace {

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void NormalizeRotation(Piece &piece, unsigned int side)
{
    piece.mRotation = side - piece.mPart;
    if (piece.mRotation < 0)
    {
        piece.mRotation += 4;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void SwapPieces(Piece &piece0, unsigned int side0, Piece &piece1, unsigned int side1)
{
    // Swap position, maintain rotation (most difficult)
    // piece0.mRotation = piece0.mRotation + SIDE_ROTATIONS[side0][side1];
    // piece1.mRotation = piece1.mRotation + SIDE_ROTATIONS[side1][side0];
    
    Piece temp = piece0;
    piece0 = piece1;
    piece1 = temp;
    
    NormalizeRotation(piece0, side0);
    NormalizeRotation(piece1, side1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int GetNumMovedPieces(bool moved[], size_t num_pieces)
{
    unsigned int num_moved = 0;
    
    for (size_t i = 0; i < num_pieces; ++i)
    {
        if (moved[i])
        {
            ++num_moved;
        }
    }
    
    return num_moved;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int GetRandomNonMovedPiece(bool moved[], size_t num_moved)
{
    Random random;
    
    unsigned int pieceIndex = random.randrange(num_moved);
    
    while (moved[pieceIndex])
    {
        pieceIndex = random.randrange(num_moved);
    }
    
    return pieceIndex;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int GetRandomOtherPiece(bool moved[], size_t num_moved, unsigned int notThisPiece)
{
    Random random;
    
    unsigned int pieceIndex = random.randrange(num_moved);
    
    while (pieceIndex / NUM_SIDES == notThisPiece / NUM_SIDES)
    {
        pieceIndex = random.randrange(num_moved);
    }
    
    return pieceIndex;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

App::App()
    : mWrappers()
    , mChannel()
    , mResetTimer(0.0f)
    , mShuffleState(SHUFFLE_STATE_START)
    , mShuffleStateTimer(kShuffleStateTimeDelay)
    , mShuffleScrambleTimer(0.0f)
    , mShuffleScoreTime(0.0f)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::Setup()
{
    for (unsigned int i = 0; i < kNumCubes; ++i)
    {
        AddCube(i);
    }
    
    mChannel.init();
    
    mShuffleState = SHUFFLE_STATE_START;
    mShuffleStateTimer = kShuffleStateTimeDelay;
    mShuffleScoreTime = 0.0;
    DEBUG_LOG(("State = START\n"));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::Tick(float dt)
{
    switch (mShuffleState)
    {
        case SHUFFLE_STATE_START:
        {
            if (mShuffleStateTimer > 0.0f)
            {
                mShuffleStateTimer -= dt;
                if (mShuffleStateTimer <= 0.0f)
                {
                    mShuffleStateTimer = 0.0f;
                    mShuffleState = SHUFFLE_STATE_SHAKE_TO_SCRAMBLE;
                    DEBUG_LOG(("State = SHAKE_TO_SCRAMBLE\n"));
                }
            }
            break;
        }
        case SHUFFLE_STATE_SCRAMBLING:
        {
            if (mShuffleScrambleTimer > 0.0f)
            {
                mShuffleScrambleTimer -= dt;
                if (mShuffleScrambleTimer <= 0.0f)
                {
                    ShufflePiece();
                    
                    if (GetNumMovedPieces(mShufflePiecesMoved, arraysize(mShufflePiecesMoved)) == arraysize(mShufflePiecesMoved))
                    {
                        mShuffleScrambleTimer = 0.0f;
                        
                        mShuffleState = SHUFFLE_STATE_UNSCRAMBLE_THE_FACES;
                        mShuffleScoreTime = 0.0f;
                        DEBUG_LOG(("State = UNSCRAMBLE_THE_FACES\n"));
                    }
                    else
                    {
                        mShuffleScrambleTimer += kShuffleScrambleTimerDelay;
                    }
                }
            }
            break;
        }
        case SHUFFLE_STATE_UNSCRAMBLE_THE_FACES:
        case SHUFFLE_STATE_PLAY:
        {
            mShuffleScoreTime += dt;
            break;
        }
        case SHUFFLE_STATE_SOLVED:
        {
            if (mShuffleStateTimer > 0.0f)
            {
                mShuffleStateTimer -= dt;
                if (mShuffleStateTimer <= 0.0f)
                {
                    mShuffleStateTimer = 0.0f;
                    mShuffleState = SHUFFLE_STATE_SCORE;
                    
                    int minutes = int(mShuffleScoreTime) / 60;
                    int seconds = int(mShuffleScoreTime - (minutes * 60.0f));
                    
                    DEBUG_LOG(("State = SCORE (Score = %02d:%02d)\n", minutes, seconds));
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }
    
    // Cube Ticks
    for (unsigned int i = 0; i < arraysize(mWrappers); ++i)
    {
        if (mWrappers[i].IsEnabled())
        {
            mWrappers[i].Tick();
        }
    }
    
    // Reset Detection
    if (AnyTouching())
    {
        mResetTimer -= dt;
        
        if (mResetTimer <= 0.0f)
        {
            mResetTimer = kResetTimerDuration;
            Reset();
        }
    }
    else
    {
        mResetTimer = kResetTimerDuration;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::Paint()
{
    for (unsigned int i = 0; i < arraysize(mWrappers); ++i)
    {
        if (mWrappers[i].IsEnabled())
        {
            mWrappers[i].Paint();
            
            switch (mShuffleState)
            {
                case SHUFFLE_STATE_SHAKE_TO_SCRAMBLE:
                {
                    mWrappers[i].PaintBanner(BannerShakeToScramble);
                    break;
                }
                case SHUFFLE_STATE_UNSCRAMBLE_THE_FACES:
                {
                    mWrappers[i].PaintBanner(BannerUnscrambleTheFaces);
                    break;
                }
                case SHUFFLE_STATE_PLAY:
                {
                    if (mWrappers[i].IsSolved())
                    {
                        mWrappers[i].PaintBanner(BannerFaceComplete);
                    }
                    break;
                }
                case SHUFFLE_STATE_SCORE:
                {
                    // TODO: Use font to print real score
                    mWrappers[i].PaintBanner(i == 0 ? BannerYourTime : BannerShakeToScramble);
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    }
    
    System::paint();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::AddCube(Cube::ID cubeId)
{
    ASSERT(cubeId < arraysize(mWrappers));
    ASSERT(!mWrappers[cubeId].IsEnabled());
    
    mWrappers[cubeId].Enable(cubeId, cubeId % kMaxBuddies);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::RemoveCube(Cube::ID cubeId)
{
    ASSERT(cubeId < arraysize(mWrappers));
    ASSERT(mWrappers[cubeId].IsEnabled());
    
    mWrappers[cubeId].Disable();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

CubeWrapper &App::GetWrapper(Cube::ID cubeId)
{
    ASSERT(cubeId < arraysize(mWrappers));
    
    return mWrappers[cubeId];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::OnNeighborAdd(Cube::ID cubeId0, Cube::Side cubeSide0, Cube::ID cubeId1, Cube::Side cubeSide1)
{
    if (!kShuffleMode ||
        mShuffleState == SHUFFLE_STATE_UNSCRAMBLE_THE_FACES ||
        mShuffleState == SHUFFLE_STATE_PLAY)
    {
        if (kShuffleMode && mShuffleState != SHUFFLE_STATE_PLAY)
        {
            mShuffleState = SHUFFLE_STATE_PLAY;
            DEBUG_LOG(("State = PLAY\n"));
        }
        
        CubeWrapper &buddy0 = GetWrapper(cubeId0);
        CubeWrapper &buddy1 = GetWrapper(cubeId1);
        
        if (buddy0.GetMode() == BUDDY_MODE_HINT || buddy1.GetMode() == BUDDY_MODE_HINT)
        {
            return;
        }
        
        Piece piece0 = buddy0.GetPiece(cubeSide0);
        Piece piece1 = buddy1.GetPiece(cubeSide1);
        
        SwapPieces(piece0, cubeSide0, piece1, cubeSide1);
        
        buddy0.SetPiece(cubeSide0, piece0);
        buddy1.SetPiece(cubeSide1, piece1);
        
        // If we have solved a face, play the sound.
        if (buddy0.IsSolved() || buddy1.IsSolved())
        {
            mChannel.play(gems1_4A9);
        }
        
        if (kShuffleMode)
        {
            if (AllSolved())
            {
                mShuffleState = SHUFFLE_STATE_SOLVED;
                mShuffleStateTimer = kShuffleStateTimeDelay;
                DEBUG_LOG(("State = SOLVED\n"));
            }
        }
        
        // Paint
        buddy0.Paint();
        buddy1.Paint();
        System::paint();
    }
}
                    
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::OnTilt(Cube::ID cubeId)
{
    if (kShuffleMode)
    {
        if(mShuffleState == SHUFFLE_STATE_UNSCRAMBLE_THE_FACES)
        {
            mShuffleState = SHUFFLE_STATE_PLAY;
            DEBUG_LOG(("State = PLAY\n"));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::OnShake(Cube::ID cubeId)
{
    if (kShuffleMode)
    {
        if( mShuffleState == SHUFFLE_STATE_SHAKE_TO_SCRAMBLE ||
            mShuffleState == SHUFFLE_STATE_SCORE)
        {
            for (unsigned int i = 0; i < arraysize(mShufflePiecesMoved); ++i)
            {
                mShufflePiecesMoved[i] = false;
            }
            
            mShuffleState = SHUFFLE_STATE_SCRAMBLING;
            DEBUG_LOG(("State = SCRAMBLING\n"));
            
            mShuffleScrambleTimer = kShuffleScrambleTimerDelay;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool App::AnyTouching() const
{
    for (unsigned int i = 0; i < arraysize(mWrappers); ++i)
    {
        if (mWrappers[i].IsEnabled() && mWrappers[i].IsTouching())
        {
            return true;
        }
    }
    
    return false;
}
                    
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool App::AllSolved() const
{
    for (unsigned int i = 0; i < arraysize(mWrappers); ++i)
    {
        if (mWrappers[i].IsEnabled() && !mWrappers[i].IsSolved())
        {
            return false;
        }
    }
    
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::Reset()
{
    for (unsigned int i = 0; i < arraysize(mWrappers); ++i)
    {
        if (mWrappers[i].IsEnabled())
        {
            mWrappers[i].Reset();
            mWrappers[i].Paint();
        }
    }
    
    mChannel.play(gems1_4A9);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::ShufflePiece()
{
    DEBUG_LOG(("ShufflePiece (%d left)\n", arraysize(mShufflePiecesMoved) - GetNumMovedPieces(mShufflePiecesMoved, arraysize(mShufflePiecesMoved))));
    
    // Pick two random pieces...
    unsigned int iPiece0 = GetRandomNonMovedPiece(mShufflePiecesMoved, arraysize(mShufflePiecesMoved));
    unsigned int iPiece1 = GetRandomOtherPiece(mShufflePiecesMoved, arraysize(mShufflePiecesMoved), iPiece0);
    
    // Swap them...
    Piece temp = mWrappers[iPiece0 / NUM_SIDES].GetPiece(iPiece0 % NUM_SIDES);
    Piece piece0 = mWrappers[iPiece1 / NUM_SIDES].GetPiece(iPiece1 % NUM_SIDES);
    Piece piece1 = temp;
    
    NormalizeRotation(piece0, iPiece0 % NUM_SIDES);
    NormalizeRotation(piece1, iPiece1 % NUM_SIDES);
    
    mWrappers[iPiece0 / NUM_SIDES].SetPiece(iPiece0 % NUM_SIDES, piece0);
    mWrappers[iPiece1 / NUM_SIDES].SetPiece(iPiece1 % NUM_SIDES, piece1);
    
    // Mark both as moved...
    mShufflePiecesMoved[iPiece0] = true;
    mShufflePiecesMoved[iPiece1] = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}
