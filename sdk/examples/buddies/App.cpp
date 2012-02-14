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

const char *kShuffleStateNames[NUM_SHUFFLE_STATES] =
{
    "SHUFFLE_STATE_START",
    "SHUFFLE_STATE_SHAKE_TO_SCRAMBLE",
    "SHUFFLE_STATE_SCRAMBLING",
    "SHUFFLE_STATE_UNSCRAMBLE_THE_FACES",
    "SHUFFLE_STATE_PLAY",
    "SHUFFLE_STATE_SOLVED",
    "SHUFFLE_STATE_SCORE",
};

const char *kSwapStateNames[NUM_SHUFFLE_STATES] =
{
    "SWAP_STATE_NONE",
    "SWAP_STATE_OUT",
    "SWAP_STATE_IN",
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

App::App()
    : mWrappers()
    , mChannel()
    , mResetTimer(0.0f)
    , mShuffleState(SHUFFLE_STATE_NONE)
    , mShuffleStateTimer(0.0f)
    , mShuffleScrambleTimer(0.0f)
    , mShuffleScoreTime(0.0f)
    , mSwapState(SWAP_STATE_NONE)
    , mSwapPiece0(0)
    , mSwapPiece1(0)
    , mSwapAnimationCounter(0)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::Init()
{
    for (unsigned int i = 0; i < kNumCubes; ++i)
    {
        AddCube(i);
    }
    
    mChannel.init();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::Setup()
{
    if (kShuffleMode)
    {
        StartShuffleState(SHUFFLE_STATE_START);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::Update(float dt)
{
    UpdateShuffle(dt);
    UpdateSwap(dt);
    
    // Cube Updates
    for (unsigned int i = 0; i < arraysize(mWrappers); ++i)
    {
        if (mWrappers[i].IsEnabled())
        {
            mWrappers[i].Update();
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

void App::Draw()
{
    for (unsigned int i = 0; i < arraysize(mWrappers); ++i)
    {
        if (mWrappers[i].IsEnabled())
        {
            // TODO: Weird that I pass in shuffle data in non-shuffle mode...
            mWrappers[i].Draw(mShuffleState, mShuffleScoreTime);
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
    bool swapping = mSwapState != SWAP_STATE_NONE;
    
    bool isHinting =
        GetWrapper(cubeId0).GetMode() == BUDDY_MODE_HINT ||
        GetWrapper(cubeId1).GetMode() == BUDDY_MODE_HINT;
    
    bool isValidShuffleState =
        mShuffleState == SHUFFLE_STATE_NONE ||
        mShuffleState == SHUFFLE_STATE_UNSCRAMBLE_THE_FACES ||
        mShuffleState == SHUFFLE_STATE_PLAY;
    
    if (!swapping && !isHinting && isValidShuffleState)
    {
        if (kShuffleMode && mShuffleState != SHUFFLE_STATE_PLAY)
        {
            StartShuffleState(SHUFFLE_STATE_PLAY);
        }
        
        SwapPieces(cubeId0 * NUM_SIDES + cubeSide0, cubeId1 * NUM_SIDES + cubeSide1);
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
            StartShuffleState(SHUFFLE_STATE_PLAY);
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
            StartShuffleState(SHUFFLE_STATE_SCRAMBLING);
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
        }
    }
    
    mChannel.play(gems1_4A9);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::StartShuffleState(ShuffleState shuffleState)
{
    mShuffleState = shuffleState;
    DEBUG_LOG(("State = %s", kShuffleStateNames[mShuffleState]));
    
    switch (shuffleState)
    {
        case SHUFFLE_STATE_START:
        {
            mShuffleStateTimer = kShuffleStateTimeDelay;
            mShuffleScoreTime = 0.0f;
            break;
        }
        case SHUFFLE_STATE_SCRAMBLING:
        {
            for (unsigned int i = 0; i < arraysize(mShufflePiecesMoved); ++i)
            {
                mShufflePiecesMoved[i] = false;
            }
            ShufflePiece();
            break;
        }
        case SHUFFLE_STATE_UNSCRAMBLE_THE_FACES:
        {
            mShuffleScoreTime = 0.0f;
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

void App::UpdateShuffle(float dt)
{
    switch (mShuffleState)
    {
        case SHUFFLE_STATE_START:
        {
            ASSERT(mShuffleStateTimer > 0.0f);
            mShuffleStateTimer -= dt;
            
            if (mShuffleStateTimer <= 0.0f)
            {
                mShuffleStateTimer = 0.0f;
                StartShuffleState(SHUFFLE_STATE_SHAKE_TO_SCRAMBLE);
            }
            break;
        }
        case SHUFFLE_STATE_SCRAMBLING:
        {
            if (mSwapAnimationCounter == 0)
            {
                ASSERT(mShuffleScrambleTimer > 0.0f);
                mShuffleScrambleTimer -= dt;
                
                if (mShuffleScrambleTimer <= 0.0f)
                {
                    mShuffleScrambleTimer = 0.0f;
                    ShufflePiece();
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
            ASSERT(mShuffleStateTimer > 0.0f);
            mShuffleStateTimer -= dt;
            
            if (mShuffleStateTimer <= 0.0f)
            {
                mShuffleStateTimer = 0.0f;
                StartShuffleState(SHUFFLE_STATE_SCORE);
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

void App::ShufflePiece()
{   
    DEBUG_LOG(("ShufflePiece... %d left\n", arraysize(mShufflePiecesMoved) - GetNumMovedPieces(mShufflePiecesMoved, arraysize(mShufflePiecesMoved))));
    
    unsigned int piece0 = GetRandomNonMovedPiece(mShufflePiecesMoved, arraysize(mShufflePiecesMoved));
    unsigned int piece1 = GetRandomOtherPiece(mShufflePiecesMoved, arraysize(mShufflePiecesMoved), piece0);
    
    SwapPieces(piece0, piece1);
    
    mShuffleScrambleTimer = kShuffleScrambleTimerDelay;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::UpdateSwap(float dt)
{
    if (mSwapState == SWAP_STATE_OUT)
    {
        ASSERT(mSwapAnimationCounter > 0);
        
        mSwapAnimationCounter -= kSwapAnimationSpeed;
        if (mSwapAnimationCounter < 0)
        {
            mSwapAnimationCounter = 0;
        }
        
        mWrappers[mSwapPiece0 / NUM_SIDES].SetPieceOffset(mSwapPiece0 % NUM_SIDES, -kSwapAnimationCount + mSwapAnimationCounter);
        mWrappers[mSwapPiece1 / NUM_SIDES].SetPieceOffset(mSwapPiece1 % NUM_SIDES, -kSwapAnimationCount + mSwapAnimationCounter);
        
        if (mSwapAnimationCounter == 0)
        {
            mSwapState = SWAP_STATE_IN;
            mSwapAnimationCounter = kSwapAnimationCount;
            
            // Swap position, maintain rotation (most difficult)
            // piece0.mRotation = piece0.mRotation + SIDE_ROTATIONS[side0][side1];
            // piece1.mRotation = piece1.mRotation + SIDE_ROTATIONS[side1][side0];
            
            Piece temp = mWrappers[mSwapPiece0 / NUM_SIDES].GetPiece(mSwapPiece0 % NUM_SIDES);
            Piece piece0 = mWrappers[mSwapPiece1 / NUM_SIDES].GetPiece(mSwapPiece1 % NUM_SIDES);
            Piece piece1 = temp;
            
            NormalizeRotation(piece0, mSwapPiece0 % NUM_SIDES);
            NormalizeRotation(piece1, mSwapPiece1 % NUM_SIDES);
            
            mWrappers[mSwapPiece0 / NUM_SIDES].SetPiece(mSwapPiece0 % NUM_SIDES, piece0);
            mWrappers[mSwapPiece1 / NUM_SIDES].SetPiece(mSwapPiece1 % NUM_SIDES, piece1);
            
            // Mark both as moved...
            mShufflePiecesMoved[mSwapPiece0] = true;
            mShufflePiecesMoved[mSwapPiece1] = true;
        }
    }
    else if (mSwapState == SWAP_STATE_IN)
    {
        ASSERT(mSwapAnimationCounter > 0);
        
        mSwapAnimationCounter -= kSwapAnimationSpeed;
        if (mSwapAnimationCounter < 0)
        {
            mSwapAnimationCounter = 0;
        }
        
        mWrappers[mSwapPiece0 / NUM_SIDES].SetPieceOffset(mSwapPiece0 % NUM_SIDES, -mSwapAnimationCounter);
        mWrappers[mSwapPiece1 / NUM_SIDES].SetPieceOffset(mSwapPiece1 % NUM_SIDES, -mSwapAnimationCounter);
        
        if (mSwapAnimationCounter == 0)
        {
            mSwapState = SWAP_STATE_NONE;
            
            if (mShuffleState == SHUFFLE_STATE_SCRAMBLING)
            {
                if (GetNumMovedPieces(mShufflePiecesMoved, arraysize(mShufflePiecesMoved)) == arraysize(mShufflePiecesMoved))
                {
                    StartShuffleState(SHUFFLE_STATE_UNSCRAMBLE_THE_FACES);
                }
                else
                {
                    mShuffleScrambleTimer += kShuffleScrambleTimerDelay;
                }
            }
            
            // If we have solved a face, play the sound.
            if (mWrappers[mSwapPiece0 / NUM_SIDES].IsSolved() || mWrappers[mSwapPiece1 / NUM_SIDES].IsSolved())
            {
                mChannel.play(gems1_4A9);
            }
            
            if (kShuffleMode)
            {
                if (AllSolved())
                {
                    StartShuffleState(SHUFFLE_STATE_SOLVED);
                    mShuffleStateTimer = kShuffleStateTimeDelay;
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::SwapPieces(unsigned int swapPiece0, unsigned int swapPiece1)
{
    mSwapPiece0 = swapPiece0;
    mSwapPiece1 = swapPiece1;
    mSwapState = SWAP_STATE_OUT;
    mSwapAnimationCounter = kSwapAnimationCount;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}
