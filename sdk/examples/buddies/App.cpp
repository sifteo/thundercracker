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

unsigned int GetRandomNonMovedPiece(
    bool moved[], size_t num_moved,
    unsigned int skip = -1)
{
    Random random;
    
    unsigned int pieceIndex = random.randrange(num_moved);
    while (moved[pieceIndex] || pieceIndex == skip)
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
            ShufflePieces();
            
            mShuffleState = SHUFFLE_STATE_SCRAMBLING;
            DEBUG_LOG(("State = SCRAMBLING\n"));
            
            // TODO: Aniamtions
            
            mShuffleState = SHUFFLE_STATE_UNSCRAMBLE_THE_FACES;
            mShuffleScoreTime = 0.0f;
            DEBUG_LOG(("State = UNSCRAMBLE_THE_FACES\n"));
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

void App::ShufflePieces()
{
    // Make a list of each part on each face
    const unsigned int kNumPieces = NUM_SIDES * kNumCubes;
    
    Piece pieces[kNumPieces];
    bool moved[kNumPieces];
    
    for (unsigned int i = 0; i < arraysize(mWrappers); ++i)
    {
        if (mWrappers[i].IsEnabled())
        {
            for (unsigned int side = 0; side < NUM_SIDES; ++side)
            {
                pieces[(i * NUM_SIDES) + side] = mWrappers[i].GetPiece(side);
                moved[(i * NUM_SIDES) + side] = false;
            }
        }
    }
    
    // While not all pieces are moved...
    while(GetNumMovedPieces(moved, arraysize(moved)) < kNumPieces)
    {
        // Pick two random pieces...
        ASSERT((kNumPieces - GetNumMovedPieces(moved, arraysize(moved))) >= 2);
        unsigned int iPiece0 = GetRandomNonMovedPiece(moved, arraysize(moved));
        unsigned int iPiece1 = GetRandomNonMovedPiece(moved, arraysize(moved), iPiece0);
        
        // Swap them...
        Piece temp = pieces[iPiece0];
        pieces[iPiece0] = pieces[iPiece1];
        pieces[iPiece1] = temp;
        
        // Mark both as moved...
        moved[iPiece0] = true;
        moved[iPiece1] = true;
    }
    
    // Copy data back into the cubes
    for (unsigned int i = 0; i < arraysize(mWrappers); ++i)
    {
        if (mWrappers[i].IsEnabled())
        {
            for (unsigned int side = 0; side < NUM_SIDES; ++side)
            {
                NormalizeRotation(pieces[(i * NUM_SIDES) + side], side);
                mWrappers[i].SetPiece(side, pieces[(i * NUM_SIDES) + side]);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}
