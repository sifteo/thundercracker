/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "App.h"
#include <sifteo.h>
#include "Config.h"
#include "Puzzle.h"
#include "PuzzleData.h"
#include "assets.gen.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace Buddies { namespace {

///////////////////////////////////////////////////////////////////////////////////////////////////
// || Various convenience functions...
// \/ 
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
    
    unsigned int pieceIndex = random.randrange(unsigned(num_moved));
    
    while (moved[pieceIndex])
    {
        pieceIndex = random.randrange(unsigned(num_moved));
    }
    
    return pieceIndex;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int GetRandomOtherPiece(bool moved[], size_t num_moved, unsigned int notThisPiece)
{
    Random random;
    
    unsigned int pieceIndex = random.randrange(unsigned(num_moved));
    
    while (pieceIndex / NUM_SIDES == notThisPiece / NUM_SIDES)
    {
        pieceIndex = random.randrange(unsigned(num_moved));
    }
    
    return pieceIndex;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool AnyTouching(App& app)
{
    for (unsigned int i = 0; i < kNumCubes; ++i)
    {
        if (app.GetCubeWrapper(i).IsEnabled() && app.GetCubeWrapper(i).IsTouching())
        {
            return true;
        }
    }
    
    return false;
}
                    
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool AllSolved(App& app)
{
    for (unsigned int i = 0; i < kNumCubes; ++i)
    {
        if (app.GetCubeWrapper(i).IsEnabled() && !app.GetCubeWrapper(i).IsSolved())
        {
            return false;
        }
    }
    
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Shuffle Mode
///////////////////////////////////////////////////////////////////////////////////////////////////

const char *kShuffleStateNames[NUM_SHUFFLE_STATES] =
{
    "SHUFFLE_STATE_NONE",
    "SHUFFLE_STATE_START",
    "SHUFFLE_STATE_SHAKE_TO_SCRAMBLE",
    "SHUFFLE_STATE_SCRAMBLING",
    "SHUFFLE_STATE_UNSCRAMBLE_THE_FACES",
    "SHUFFLE_STATE_PLAY",
    "SHUFFLE_STATE_SOLVED",
    "SHUFFLE_STATE_SCORE",
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Authored Mode
///////////////////////////////////////////////////////////////////////////////////////////////////

const char *kAuthoredStateNames[NUM_AUTHORED_STATES] =
{
    "AUTHORED_STATE_NONE",
    "AUTHORED_STATE_START",
    "AUTHORED_STATE_INSTRUCTIONS",
    "AUTHORED_STATE_PLAY",
    "AUTHORED_STATE_SOLVED",
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

const int kSwapAnimationCount = 64 - 8; // Note: sprites are offset by 8 pixels by design

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

App::App()
    : mCubeWrappers()
    , mChannel()
    , mResetTimer(0.0f)
    , mDelayTimer(0.0f)
    , mSwapState(SWAP_STATE_NONE)
    , mSwapPiece0(0)
    , mSwapPiece1(0)
    , mSwapAnimationCounter(0)
    , mShuffleState(SHUFFLE_STATE_NONE)
    , mShuffleMoveCounter(0)
    , mShuffleScoreTime(0.0f)
    , mAuthoredState(AUTHORED_STATE_NONE)
    , mAuthoredPuzzleIndex(0)
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

void App::Reset()
{
    ResetCubes();
    
    if (kGameMode == GAME_MODE_SHUFFLE)
    {
        StartShuffleState(SHUFFLE_STATE_START);
    }
    else if (kGameMode == GAME_MODE_AUTHORED)
    {
        StartAuthoredState(AUTHORED_STATE_START);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::Update(float dt)
{
    UpdateShuffle(dt);
    UpdateAuthored(dt);
    UpdateSwap(dt);
    
    // Cubes
    for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
    {
        if (mCubeWrappers[i].IsEnabled())
        {
            mCubeWrappers[i].Update(dt);
        }
    }
    
    // Reset Detection
    if (AnyTouching(*this))
    {
        mResetTimer -= dt;
        
        if (mResetTimer <= 0.0f)
        {
            mResetTimer = kResetTimerDuration;
            
            PlaySound();
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
    for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
    {
        if (mCubeWrappers[i].IsEnabled())
        {
            mCubeWrappers[i].Draw();
            
            if (mAuthoredState == AUTHORED_STATE_INSTRUCTIONS)
            {
                ASSERT(mAuthoredPuzzleIndex < GetNumPuzzles());
                mCubeWrappers[i].DrawTextBanner(GetPuzzle(mAuthoredPuzzleIndex).GetInstructions());
            }
            else if (mShuffleState != SHUFFLE_STATE_NONE)
            {
                mCubeWrappers[i].DrawShuffleUi(mShuffleState, mShuffleScoreTime);
            }
        }
    }
    
    System::paint();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

CubeWrapper &App::GetCubeWrapper(Cube::ID cubeId)
{
    ASSERT(cubeId < arraysize(mCubeWrappers));
    
    return mCubeWrappers[cubeId];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::OnNeighborAdd(Cube::ID cubeId0, Cube::Side cubeSide0, Cube::ID cubeId1, Cube::Side cubeSide1)
{
    bool isSwapping = mSwapState != SWAP_STATE_NONE;
    
    bool isHinting =
        GetCubeWrapper(cubeId0).IsHinting() ||
        GetCubeWrapper(cubeId1).IsHinting();
    
    bool isFixed =
        GetCubeWrapper(cubeId0).GetPiece(cubeSide0).mAttribute == Piece::ATTRIBUTE_FIXED ||
        GetCubeWrapper(cubeId1).GetPiece(cubeSide1).mAttribute == Piece::ATTRIBUTE_FIXED;
    
    bool isValidShuffleState =
        mShuffleState == SHUFFLE_STATE_NONE ||
        mShuffleState == SHUFFLE_STATE_UNSCRAMBLE_THE_FACES ||
        mShuffleState == SHUFFLE_STATE_PLAY;
    
    bool isValidAuthoredState =
        mAuthoredState == AUTHORED_STATE_NONE ||
        mAuthoredState == AUTHORED_STATE_INSTRUCTIONS ||
        mAuthoredState == AUTHORED_STATE_PLAY;
    
    if (!isSwapping && !isHinting && !isFixed && isValidShuffleState && isValidAuthoredState)
    {
        if (kGameMode == GAME_MODE_SHUFFLE && mShuffleState != SHUFFLE_STATE_PLAY)
        {
            StartShuffleState(SHUFFLE_STATE_PLAY);
        }
        else if (kGameMode == GAME_MODE_AUTHORED && mAuthoredState != AUTHORED_STATE_PLAY)
        {
            StartAuthoredState(AUTHORED_STATE_PLAY);
        }
        
        OnSwapBegin(cubeId0 * NUM_SIDES + cubeSide0, cubeId1 * NUM_SIDES + cubeSide1);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::OnTilt(Cube::ID cubeId)
{
    if (kGameMode == GAME_MODE_SHUFFLE)
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
    if (kGameMode == GAME_MODE_SHUFFLE)
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

void App::AddCube(Cube::ID cubeId)
{
    ASSERT(cubeId < arraysize(mCubeWrappers));
    ASSERT(!mCubeWrappers[cubeId].IsEnabled());
    
    mCubeWrappers[cubeId].Enable(cubeId, cubeId % kMaxBuddies);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::RemoveCube(Cube::ID cubeId)
{
    ASSERT(cubeId < arraysize(mCubeWrappers));
    ASSERT(mCubeWrappers[cubeId].IsEnabled());
    
    mCubeWrappers[cubeId].Disable();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::ResetCubes()
{
    for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
    {
        if (mCubeWrappers[i].IsEnabled())
        {
            mCubeWrappers[i].Reset();
            
            for (unsigned int j = 0; j < NUM_SIDES; ++j)
            {
                if (kGameMode == GAME_MODE_AUTHORED)
                {
                    ASSERT(mAuthoredPuzzleIndex < GetNumPuzzles());
                    mCubeWrappers[i].SetPiece(j, GetPuzzle(mAuthoredPuzzleIndex).GetStartState(i, j));
                    mCubeWrappers[i].SetPieceSolution(j, GetPuzzle(mAuthoredPuzzleIndex).GetEndState(i, j));
                }
                else
                {
                    mCubeWrappers[i].SetPiece(j, GetPuzzleDefault().GetStartState(i, j));
                    mCubeWrappers[i].SetPieceSolution(j, GetPuzzleDefault().GetEndState(i, j));
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::PlaySound()
{
    mChannel.play(GemsSound);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::StartShuffleState(ShuffleState shuffleState)
{
    mShuffleState = shuffleState;
    DEBUG_LOG(("State = %s\n", kShuffleStateNames[mShuffleState]));
    
    switch (mShuffleState)
    {
        case SHUFFLE_STATE_START:
        {
            mDelayTimer = kShuffleStateTimeDelay;
            break;
        }
        case SHUFFLE_STATE_SCRAMBLING:
        {
            mShuffleMoveCounter = 0;
            for (unsigned int i = 0; i < arraysize(mShufflePiecesMoved); ++i)
            {
                mShufflePiecesMoved[i] = false;
            }
            ShufflePieces();
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
            ASSERT(mDelayTimer > 0.0f);
            mDelayTimer -= dt;
            
            if (mDelayTimer <= 0.0f)
            {
                mDelayTimer = 0.0f;
                StartShuffleState(SHUFFLE_STATE_SHAKE_TO_SCRAMBLE);
            }
            break;
        }
        case SHUFFLE_STATE_SCRAMBLING:
        {
            if (mSwapAnimationCounter == 0)
            {
                ASSERT(mDelayTimer > 0.0f);
                mDelayTimer -= dt;
                
                if (mDelayTimer <= 0.0f)
                {
                    mDelayTimer = 0.0f;
                    ShufflePieces();
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
            ASSERT(mDelayTimer > 0.0f);
            mDelayTimer -= dt;
            
            if (mDelayTimer <= 0.0f)
            {
                mDelayTimer = 0.0f;
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

void App::StartAuthoredState(AuthoredState authoredState)
{
    mAuthoredState = authoredState;
    DEBUG_LOG(("State = %s\n", kAuthoredStateNames[mAuthoredState]));
    
    switch (mAuthoredState)
    {
        case AUTHORED_STATE_START:
        {
            mAuthoredPuzzleIndex = 0;
            mDelayTimer = kShuffleStateTimeDelay;
            break;
        }
        case AUTHORED_STATE_PLAY:
        {
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    mCubeWrappers[i].ClearBg1();
                }
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

void App::UpdateAuthored(float dt)
{
    switch (mAuthoredState)
    {
        case AUTHORED_STATE_START:
        {
            ASSERT(mDelayTimer > 0.0f);
            mDelayTimer -= dt;
            
            if (mDelayTimer <= 0.0f)
            {
                mDelayTimer = 0.0f;
                StartAuthoredState(AUTHORED_STATE_INSTRUCTIONS);
            }
            break;
        }
        case AUTHORED_STATE_SOLVED:
        {
            ASSERT(mDelayTimer > 0.0f);
            mDelayTimer -= dt;
            
            if (mDelayTimer <= 0.0f)
            {
                mDelayTimer = 0.0f;
                
                if (++mAuthoredPuzzleIndex == GetNumPuzzles())
                {
                    mAuthoredPuzzleIndex = 0;
                }
                ResetCubes();
                
                StartAuthoredState(AUTHORED_STATE_INSTRUCTIONS);
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

void App::ShufflePieces()
{   
    DEBUG_LOG(("ShufflePiece... %d left\n", arraysize(mShufflePiecesMoved) - GetNumMovedPieces(mShufflePiecesMoved, arraysize(mShufflePiecesMoved))));
    
    ++mShuffleMoveCounter;
    
    unsigned int piece0 = GetRandomNonMovedPiece(mShufflePiecesMoved, arraysize(mShufflePiecesMoved));
    unsigned int piece1 = GetRandomOtherPiece(mShufflePiecesMoved, arraysize(mShufflePiecesMoved), piece0);
    
    OnSwapBegin(piece0, piece1);
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
        
        mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPieceOffset(mSwapPiece0 % NUM_SIDES, -kSwapAnimationCount + mSwapAnimationCounter);
        mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPieceOffset(mSwapPiece1 % NUM_SIDES, -kSwapAnimationCount + mSwapAnimationCounter);
        
        if (mSwapAnimationCounter == 0)
        {
            OnSwapExchange();
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
        
        mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPieceOffset(mSwapPiece0 % NUM_SIDES, -mSwapAnimationCounter);
        mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPieceOffset(mSwapPiece1 % NUM_SIDES, -mSwapAnimationCounter);
        
        if (mSwapAnimationCounter == 0)
        {
            OnSwapFinish();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::OnSwapBegin(unsigned int swapPiece0, unsigned int swapPiece1)
{
    mSwapPiece0 = swapPiece0;
    mSwapPiece1 = swapPiece1;
    mSwapState = SWAP_STATE_OUT;
    mSwapAnimationCounter = kSwapAnimationCount;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////              

void App::OnSwapExchange()
{
    mSwapState = SWAP_STATE_IN;
    mSwapAnimationCounter = kSwapAnimationCount;
    
    Piece temp = mCubeWrappers[mSwapPiece0 / NUM_SIDES].GetPiece(mSwapPiece0 % NUM_SIDES);
    Piece piece0 = mCubeWrappers[mSwapPiece1 / NUM_SIDES].GetPiece(mSwapPiece1 % NUM_SIDES);
    Piece piece1 = temp;
    
    mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPiece(mSwapPiece0 % NUM_SIDES, piece0);
    mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPiece(mSwapPiece1 % NUM_SIDES, piece1);
    
    // Mark both as moved...
    mShufflePiecesMoved[mSwapPiece0] = true;
    mShufflePiecesMoved[mSwapPiece1] = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::OnSwapFinish()
{
    mSwapState = SWAP_STATE_NONE;
    
    if (mShuffleState == SHUFFLE_STATE_SCRAMBLING)
    {
        bool done = GetNumMovedPieces(mShufflePiecesMoved, arraysize(mShufflePiecesMoved)) == arraysize(mShufflePiecesMoved);
        if (kShuffleMaxMoves > 0)
        {
            done = done || mShuffleMoveCounter == kShuffleMaxMoves;
        }
    
        if (done)
        {
            StartShuffleState(SHUFFLE_STATE_UNSCRAMBLE_THE_FACES);
        }
        else
        {
            mDelayTimer += kShuffleScrambleTimerDelay;
        }
    }
    else if (mShuffleState == SHUFFLE_STATE_PLAY)
    {
        if (AllSolved(*this))
        {
            StartShuffleState(SHUFFLE_STATE_SOLVED);
            mDelayTimer = kShuffleStateTimeDelay;
        }
    }
    else if (mAuthoredState == AUTHORED_STATE_PLAY)
    {
        if (AllSolved(*this))
        {
            StartAuthoredState(AUTHORED_STATE_SOLVED);
            mDelayTimer = kShuffleStateTimeDelay;
        }
    }
    
    if (mCubeWrappers[mSwapPiece0 / NUM_SIDES].IsSolved() ||
        mCubeWrappers[mSwapPiece1 / NUM_SIDES].IsSolved())
    {
        PlaySound();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}
