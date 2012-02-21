/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "App.h"
#include <sifteo/string.h>
#include <sifteo/system.h>
#include "Config.h"
#include "Puzzle.h"
#include "PuzzleData.h"
#include "assets.gen.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

using namespace Sifteo;

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace Buddies { namespace {

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int GetNumMovedPieces(bool moved[], unsigned int num_pieces)
{
    unsigned int num_moved = 0;
    
    for (unsigned int i = 0; i < num_pieces; ++i)
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

unsigned int GetRandomNonMovedPiece(bool moved[], unsigned int num_moved)
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

unsigned int GetRandomOtherPiece(bool moved[], unsigned int num_moved, unsigned int notThisPiece)
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
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawChapterTitle(CubeWrapper &cubeWrapper, unsigned int puzzleIndex)
{
    String<128> buffer;
    buffer << "Chapter " << (puzzleIndex + 1) << "\n" << "\"" << GetPuzzle(puzzleIndex).GetChapterTitle() << "\"";
    cubeWrapper.DrawBackgroundWithText(ChapterTitle, buffer.c_str(), Vec2(3, 4));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawChapterSummary(
    CubeWrapper &cubeWrapper,
    unsigned int puzzleIndex,
    float scoreTime,
    unsigned int scoreMoves)
{
    int minutes = int(scoreTime) / 60;
    int seconds = int(scoreTime - (minutes * 60.0f));
    
    String<128> buffer;
    buffer << "Chapter " << (puzzleIndex + 1) << "\nTime:" << Fixed(minutes, 2, true) << ":" << Fixed(seconds, 2, true) << "\nMoves:" << scoreMoves;
    
    cubeWrapper.DrawBackgroundWithText(ChapterSummary, buffer.c_str(), Vec2(3, 4));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawChapterNext(CubeWrapper &cubeWrapper, unsigned int puzzleIndex)
{
    unsigned int nextPuzzleIndex = ++puzzleIndex % GetNumPuzzles();
    
    String<128> buffer;
    buffer << "Chapter " << (nextPuzzleIndex + 1);
    cubeWrapper.DrawBackgroundWithText(ChapterNext, buffer.c_str(), Vec2(3, 8));
}
                    
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawRetry(CubeWrapper &cubeWrapper, unsigned int puzzleIndex)
{
    String<128> buffer;
    buffer << "Chapter " << (puzzleIndex + 1);
    cubeWrapper.DrawBackgroundWithText(Retry, buffer.c_str(), Vec2(3, 8));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

const char *kGameStateNames[NUM_GAME_STATES] =
{
    "SHUFFLE_STATE_NONE",
    "SHUFFLE_STATE_START",
    "GAME_STATE_SHUFFLE_SHAKE_TO_SCRAMBLE",
    "GAME_STATE_SHUFFLE_SCRAMBLING",
    "GAME_STATE_SHUFFLE_UNSCRAMBLE_THE_FACES",
    "GAME_STATE_SHUFFLE_PLAY",
    "GAME_STATE_SHUFFLE_SOLVED",
    "GAME_STATE_SHUFFLE_SCORE",
    "GAME_STATE_STORY_START",
    "GAME_STATE_STORY_INSTRUCTIONS",
    "GAME_STATE_STORY_PLAY",
    "GAME_STATE_STORY_SOLVED",
};

const int kSwapAnimationCount = 64 - 8; // Note: sprites are offset by 8 pixels by design

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

App::App()
    : mCubeWrappers()
    , mChannel()
    , mGameState(GAME_STATE_NONE)
    , mResetTimer(0.0f)
    , mDelayTimer(0.0f)
    , mTouching(false)
    , mScoreTimer(0.0f)
    , mScoreMoves(0)
    , mSwapState(SWAP_STATE_NONE)
    , mSwapPiece0(0)
    , mSwapPiece1(0)
    , mSwapAnimationCounter(0)
    , mHintPieceSkip(-1)
    , mHintPiece0(-1)
    , mHintPiece1(-1)
    , mHintBlinkTimer(kHintBlinkTimerDuration)
    , mHintBlinking(false)
    , mShuffleMoveCounter(0)
    , mShuffleHintTimer(0.0f)
    , mPuzzleIndex(0)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::Init()
{
    // TODO: This causes loading assets if kNumCubes > the actual number of connected cubes.
    // Is there anyway we can see how many are really connected before adding them?
    for (unsigned int i = 0; i < kNumCubes; ++i)
    {
        AddCube(i);
    }
    
#ifdef SIFTEO_SIMULATOR
    mChannel.init();
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::Reset()
{
    switch (kGameMode)
    {
        default:
        case GAME_MODE_FREE_PLAY:
        {
            StartGameState(GAME_STATE_FREE_PLAY);
            break;
        }
        case GAME_MODE_SHUFFLE:
        {
            StartGameState(GAME_STATE_SHUFFLE_START);
            break;
        }
        case GAME_MODE_STORY:
        {
            StartGameState(GAME_STATE_STORY_START);
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::Update(float dt)
{
    UpdateGameState(dt);
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
    DrawGameState();
    
    // TODO: paintSync() handling
    System::paint();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

const CubeWrapper &App::GetCubeWrapper(Cube::ID cubeId) const
{
    ASSERT(cubeId < arraysize(mCubeWrappers));
    
    return mCubeWrappers[cubeId];
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

void App::OnNeighborAdd(
    Cube::ID cubeId0, Cube::Side cubeSide0,
    Cube::ID cubeId1, Cube::Side cubeSide1)
{
    if (mGameState == GAME_STATE_STORY_CLUE)
    {
        StartGameState(GAME_STATE_STORY_PLAY);
    }
    else if (mGameState == GAME_STATE_STORY_HINT_1 || mGameState == GAME_STATE_STORY_HINT_2)
    {
        StartGameState(GAME_STATE_STORY_PLAY);
    }
    else
    {
        bool isSwapping = mSwapState != SWAP_STATE_NONE;
        
        bool isFixed =
            GetCubeWrapper(cubeId0).GetPiece(cubeSide0).mAttribute == Piece::ATTR_FIXED ||
            GetCubeWrapper(cubeId1).GetPiece(cubeSide1).mAttribute == Piece::ATTR_FIXED;
        
        bool isValidGameState =
            mGameState == GAME_STATE_FREE_PLAY ||
            mGameState == GAME_STATE_SHUFFLE_UNSCRAMBLE_THE_FACES ||
            mGameState == GAME_STATE_SHUFFLE_PLAY ||
            mGameState == GAME_STATE_STORY_PLAY;
        
        bool isValidCube =
            kGameMode != GAME_MODE_STORY ||
            (   cubeId0 < GetPuzzle(mPuzzleIndex).GetNumBuddies() &&
                cubeId1 < GetPuzzle(mPuzzleIndex).GetNumBuddies());
        
        if (!isSwapping && !isFixed && isValidGameState && isValidCube)
        {
            if (kGameMode == GAME_MODE_SHUFFLE && mGameState != GAME_STATE_SHUFFLE_PLAY)
            {
                StartGameState(GAME_STATE_SHUFFLE_PLAY);
            }
            
            ++mScoreMoves;
            OnSwapBegin(cubeId0 * NUM_SIDES + cubeSide0, cubeId1 * NUM_SIDES + cubeSide1);
        }
    }
    
    mShuffleHintTimer = kHintTimerOnDuration;
    mHintPiece0 = -1;
    mHintPiece1 = -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::OnTilt(Cube::ID cubeId)
{
    if(mGameState == GAME_STATE_SHUFFLE_UNSCRAMBLE_THE_FACES)
    {
        StartGameState(GAME_STATE_SHUFFLE_PLAY);
    }
    else if (mGameState == GAME_STATE_STORY_CLUE)
    {
        StartGameState(GAME_STATE_STORY_PLAY);
    }
    else if (mGameState == GAME_STATE_STORY_HINT_1 || mGameState == GAME_STATE_STORY_HINT_2)
    {
        StartGameState(GAME_STATE_STORY_PLAY);
    }
    
    mShuffleHintTimer = kHintTimerOnDuration;
    mHintPiece0 = -1;
    mHintPiece1 = -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::OnShake(Cube::ID cubeId)
{
    if( mGameState == GAME_STATE_SHUFFLE_SHAKE_TO_SCRAMBLE ||
        mGameState == GAME_STATE_SHUFFLE_SCORE)
    {
        StartGameState(GAME_STATE_SHUFFLE_SCRAMBLING);
    }
    else if (mGameState == GAME_STATE_STORY_CLUE)
    {
        StartGameState(GAME_STATE_STORY_PLAY);
    }
    else if (mGameState == GAME_STATE_STORY_HINT_1 || mGameState == GAME_STATE_STORY_HINT_2)
    {
        StartGameState(GAME_STATE_STORY_PLAY);
    }
    
    mShuffleHintTimer = kHintTimerOnDuration;
    mHintPiece0 = -1;
    mHintPiece1 = -1;
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

void App::ResetCubesToPuzzle(const Puzzle &puzzle)
{
    ASSERT(kNumCubes >= GetPuzzle(mPuzzleIndex).GetNumBuddies());
    
    for (unsigned int i = 0; i < puzzle.GetNumBuddies(); ++i)
    {
        ASSERT(i < arraysize(mCubeWrappers));
        if (mCubeWrappers[i].IsEnabled())
        {
            mCubeWrappers[i].Reset();
            
            for (unsigned int j = 0; j < NUM_SIDES; ++j)
            {
                mCubeWrappers[i].SetPiece(j, puzzle.GetStartState(i, j));
                mCubeWrappers[i].SetPieceSolution(j, puzzle.GetEndState(i, j));
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::PlaySound()
{
#ifdef SIFTEO_SIMULATOR
    mChannel.play(GemsSound);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::StartGameState(GameState gameState)
{
    mGameState = gameState;
    
    ASSERT(gameState < int(arraysize(kGameStateNames)));
    DEBUG_LOG(("State = %s\n", kGameStateNames[mGameState]));
    
    switch (mGameState)
    {
        case GAME_STATE_FREE_PLAY:
        {
            ResetCubesToPuzzle(GetPuzzleDefault());
        }
        case GAME_STATE_SHUFFLE_START:
        {
            ResetCubesToPuzzle(GetPuzzleDefault());
            mDelayTimer = kStateTimeDelayShort;
            mShuffleHintTimer = kHintTimerOnDuration;
            mHintPiece0 = -1;
            mHintPiece1 = -1;
            mTouching = false;
            break;
        }
        case GAME_STATE_SHUFFLE_SCRAMBLING:
        {
            mShuffleMoveCounter = 0;
            for (unsigned int i = 0; i < arraysize(mShufflePiecesMoved); ++i)
            {
                mShufflePiecesMoved[i] = false;
            }
            ShufflePieces();
            break;
        }
        case GAME_STATE_SHUFFLE_UNSCRAMBLE_THE_FACES:
        {
            mScoreTimer = 0.0f;
            mScoreMoves = 0;
            break;
        }
        case GAME_STATE_SHUFFLE_SOLVED:
        {
            mHintPiece0 = -1;
            mHintPiece1 = -1;
            break;
        }
        case GAME_STATE_STORY_START:
        {
            mPuzzleIndex = 0;
            StartGameState(GAME_STATE_STORY_CHAPTER_START);
            break;
        }
        case GAME_STATE_STORY_CHAPTER_START:
        {
            ASSERT(mPuzzleIndex < GetNumPuzzles());
            ResetCubesToPuzzle(GetPuzzle(mPuzzleIndex));
            mDelayTimer = kStateTimeDelayLong;
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_START:
        {
            mDelayTimer = kStateTimeDelayLong;
            break;
        }
        case GAME_STATE_STORY_DISPLAY_START_STATE:
        {
            mDelayTimer = kStateTimeDelayShort;
            break;
        }
        case GAME_STATE_STORY_PLAY:
        {
            mScoreTimer = 0.0f;
            mScoreMoves = 0;
            break;
        }
        case GAME_STATE_STORY_HINT_1:
        {
            mDelayTimer = kStateTimeDelayLong;
            break;
        }
        case GAME_STATE_STORY_HINT_2:
        {
            ChooseHint();
            mHintBlinkTimer = kHintBlinkTimerDuration;
            break;
        }
        case GAME_STATE_STORY_SOLVED:
        {
            mDelayTimer = kStateTimeDelayShort;
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_END:
        {
            mDelayTimer = kStateTimeDelayLong;
            break;
        }
        case GAME_STATE_STORY_CHAPTER_END:
        {
            break;
        }
        default:
        {
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TODO: Clean up repeated mDelayTimer and AnyTouching code.
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::UpdateGameState(float dt)
{
    switch (mGameState)
    {
        case GAME_STATE_SHUFFLE_START:
        {
            ASSERT(mDelayTimer > 0.0f);
            mDelayTimer -= dt;
            
            if (mDelayTimer <= 0.0f)
            {
                mDelayTimer = 0.0f;
                StartGameState(GAME_STATE_SHUFFLE_SHAKE_TO_SCRAMBLE);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_SCRAMBLING:
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
        case GAME_STATE_SHUFFLE_UNSCRAMBLE_THE_FACES:
        {
            mScoreTimer += dt; // TODO: Should we count time here?
            
            if (AnyTouching(*this))
            {
                mTouching = true;
                StartGameState(GAME_STATE_SHUFFLE_PLAY);
            }
            
            break;
        }
        case GAME_STATE_SHUFFLE_PLAY:
        {
            mScoreTimer += dt;
            
            if (mShuffleHintTimer > 0.0f)
            {
                mShuffleHintTimer -= dt;
                if (mShuffleHintTimer <= 0.0f)
                {
                    mShuffleHintTimer = 0.0f;
                    ChooseHint();
                }
            }
            
            if (mTouching)
            {
                if (!AnyTouching(*this))
                {
                    mTouching = false;
                    mShuffleHintTimer = kHintTimerOnDuration;
                    mHintPiece0 = -1;
                    mHintPiece1 = -1;

                }
            }
            else
            {
                if (AnyTouching(*this))
                {
                    mTouching = true;
                    ChooseHint();
                }
            }
            
            break;
        }
        case GAME_STATE_SHUFFLE_SOLVED:
        {
            ASSERT(mDelayTimer > 0.0f);
            mDelayTimer -= dt;
            
            if (mDelayTimer <= 0.0f)
            {
                mDelayTimer = 0.0f;
                StartGameState(GAME_STATE_SHUFFLE_SCORE);
            }
            break;
        }
        case GAME_STATE_STORY_CHAPTER_START:
        {
            ASSERT(mDelayTimer > 0.0f);
            mDelayTimer -= dt;
            
            if (mDelayTimer <= 0.0f)
            {
                mDelayTimer = 0.0f;
                StartGameState(GAME_STATE_STORY_CUTSCENE_START);
            }
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_START:
        {
            ASSERT(mDelayTimer > 0.0f);
            mDelayTimer -= dt;
            
            if (mDelayTimer <= 0.0f)
            {
                mDelayTimer = 0.0f;
                StartGameState(GAME_STATE_STORY_DISPLAY_START_STATE);
            }
            break;
        }
        case GAME_STATE_STORY_DISPLAY_START_STATE:
        {
            ASSERT(mDelayTimer > 0.0f);
            mDelayTimer -= dt;
            
            if (mDelayTimer <= 0.0f)
            {
                mDelayTimer = 0.0f;
                StartGameState(GAME_STATE_STORY_CLUE);
            }
            break;
        }
        case GAME_STATE_STORY_CLUE:
        {
            if (!mTouching)
            {
                if (AnyTouching(*this))
                {
                    mTouching = true;
                    StartGameState(GAME_STATE_STORY_PLAY);
                }
            }
            else
            {
                if (!AnyTouching(*this))
                {
                    mTouching = false;
                }
            }
            
            break;
        }
        case GAME_STATE_STORY_PLAY:
        {
            mScoreTimer += dt;
            
            if (!mTouching)
            {
                for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
                {
                    if (mCubeWrappers[i].IsEnabled() && mCubeWrappers[i].IsTouching())
                    {
                        mTouching = true;
                        StartGameState(GAME_STATE_STORY_HINT_1);
                        break;
                    }
                }
            }
            else
            {
                if (!AnyTouching(*this))
                {
                    mTouching = false;
                }
            }
            break;
        }
        case GAME_STATE_STORY_HINT_1:
        {
            mScoreTimer += dt;
            
            ASSERT(mDelayTimer > 0.0f);
            mDelayTimer -= dt;
            
            if (mDelayTimer <= 0.0f)
            {
                mDelayTimer = 0.0f;
                StartGameState(GAME_STATE_STORY_PLAY);
            }
            
            if (!mTouching)
            {
                for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
                {
                    if (mCubeWrappers[i].IsEnabled() && mCubeWrappers[i].IsTouching())
                    {
                        mTouching = true;
                        StartGameState(GAME_STATE_STORY_HINT_2);
                        break;
                    }
                }
            }
            else
            {
                if (!AnyTouching(*this))
                {
                    mTouching = false;
                }
            }
            break;
        }
        case GAME_STATE_STORY_HINT_2:
        {
            mScoreTimer += dt;
            
            ASSERT(mHintBlinkTimer > 0.0f);
            mHintBlinkTimer -= dt;
            if (mHintBlinkTimer <= 0.0f)
            {
                mHintBlinkTimer += kHintBlinkTimerDuration;
                mHintBlinking = !mHintBlinking;
            }
            
            if (!mTouching)
            {
                for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
                {
                    if (mCubeWrappers[i].IsEnabled() && mCubeWrappers[i].IsTouching())
                    {
                        mTouching = true;
                        StartGameState(GAME_STATE_STORY_PLAY);
                        break;
                    }
                }
            }
            else
            {
                if (!AnyTouching(*this))
                {
                    mTouching = false;
                }
            }
            break;
        }
        case GAME_STATE_STORY_SOLVED:
        {
            ASSERT(mDelayTimer > 0.0f);
            mDelayTimer -= dt;
            
            if (mDelayTimer <= 0.0f)
            {
                mDelayTimer = 0.0f;
                StartGameState(GAME_STATE_STORY_CUTSCENE_END);
            }
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_END:
        {
            ASSERT(mDelayTimer > 0.0f);
            mDelayTimer -= dt;
            
            if (mDelayTimer <= 0.0f)
            {
                mDelayTimer = 0.0f;
                StartGameState(GAME_STATE_STORY_CHAPTER_END);
            }
            break;
        }
        case GAME_STATE_STORY_CHAPTER_END:
        {
            if (!mTouching)
            {
                for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
                {
                    if (mCubeWrappers[i].IsEnabled() && mCubeWrappers[i].IsTouching())
                    {
                        mTouching = true;
                        if (i == 0)
                        {
                            if (++mPuzzleIndex == GetNumPuzzles())
                            {
                                mPuzzleIndex = 0;
                            }
                            StartGameState(GAME_STATE_STORY_CHAPTER_START);
                        }
                        else if (i == 1)
                        {
                            StartGameState(GAME_STATE_STORY_CHAPTER_START);
                        }
                        else if (i == 2)
                        {
                            // TODO: go to main menu
                        }
                        break;
                    }
                }
            }
            else
            {
                if (!AnyTouching(*this))
                {
                    mTouching = false;
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
// TODO: Pull for loop to outside
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::DrawGameState()
{
    if (kGameMode == GAME_MODE_FREE_PLAY)
    {
        for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
        {
            if (mCubeWrappers[i].IsEnabled())
            {
                mCubeWrappers[i].DrawBuddy();
            }
        }
    }
    else if (kGameMode == GAME_MODE_STORY)
    {
        if (mGameState == GAME_STATE_STORY_CHAPTER_START)
        {
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    DrawChapterTitle(mCubeWrappers[i], mPuzzleIndex);
                }
            }
        }
        else if (mGameState == GAME_STATE_STORY_CUTSCENE_START)
        {
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    if (i == 0)
                    {
                        mCubeWrappers[i].DrawCutscene(GetPuzzle(mPuzzleIndex).GetCutsceneTextStart());
                    }
                    else
                    {
                        DrawChapterTitle(mCubeWrappers[i], mPuzzleIndex);
                    }
                }
            }
        }
        else if (mGameState == GAME_STATE_STORY_DISPLAY_START_STATE)
        {
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    if (i < GetPuzzle(mPuzzleIndex).GetNumBuddies())
                    {
                        mCubeWrappers[i].DrawBuddy();
                    }
                    else
                    {
                        DrawChapterTitle(mCubeWrappers[i], mPuzzleIndex);
                    }
                }
            }
        }
        else if (mGameState == GAME_STATE_STORY_CLUE)
        {
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    if (i < GetPuzzle(mPuzzleIndex).GetNumBuddies())
                    {
                        mCubeWrappers[i].DrawBuddy();
                        mCubeWrappers[i].DrawClue(GetPuzzle(mPuzzleIndex).GetClue());
                    }
                    else
                    {
                        DrawChapterTitle(mCubeWrappers[i], mPuzzleIndex);
                    }
                }
            }
        }
        else if (mGameState == GAME_STATE_STORY_PLAY)
        {
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    if (i < GetPuzzle(mPuzzleIndex).GetNumBuddies())
                    {
                        mCubeWrappers[i].DrawBuddy();
                    }
                    else
                    {
                        DrawChapterTitle(mCubeWrappers[i], mPuzzleIndex);
                    }
                }
            }
        }
        else if (mGameState == GAME_STATE_STORY_HINT_1)
        {
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    if (i < GetPuzzle(mPuzzleIndex).GetNumBuddies())
                    {
                        mCubeWrappers[i].DrawBuddy();
                        
                        if (i == 0)
                        {
                            mCubeWrappers[i].DrawClue(GetPuzzle(mPuzzleIndex).GetClue(), true);
                        }
                    }
                    else
                    {
                        DrawChapterTitle(mCubeWrappers[i], mPuzzleIndex);
                    }
                }
            }
        }
        else if (mGameState == GAME_STATE_STORY_HINT_2)
        {
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    if (i < GetPuzzle(mPuzzleIndex).GetNumBuddies())
                    {
                        if (mHintPiece0 != -1 && (mHintPiece0 / NUM_SIDES) == int(i))
                        {
                            mCubeWrappers[i].DrawBuddyWithStoryHint(mHintPiece0 % NUM_SIDES, mHintBlinking);
                        }
                        else if (mHintPiece1 != -1 && (mHintPiece1 / NUM_SIDES) == int(i))
                        {
                            mCubeWrappers[i].DrawBuddyWithStoryHint(mHintPiece1 % NUM_SIDES, mHintBlinking);
                        }
                        else
                        {
                            mCubeWrappers[i].DrawBuddy();
                        }
                    }
                    else
                    {
                        DrawChapterTitle(mCubeWrappers[i], mPuzzleIndex);
                    }
                }
            }
        }
        else if (mGameState == GAME_STATE_STORY_SOLVED)
        {
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    if (i < GetPuzzle(mPuzzleIndex).GetNumBuddies())
                    {
                        mCubeWrappers[i].DrawBuddy();
                    }
                    else
                    {
                        DrawChapterTitle(mCubeWrappers[i], mPuzzleIndex);
                    }
                }
            }
        }
        else if (mGameState == GAME_STATE_STORY_CUTSCENE_END)
        {
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    if (i == 0)
                    {
                        mCubeWrappers[i].DrawCutscene(GetPuzzle(mPuzzleIndex).GetCutsceneTextEnd());
                    }
                    else
                    {
                        DrawChapterSummary(mCubeWrappers[i], mPuzzleIndex, mScoreTimer, mScoreMoves);
                    }
                }
            }
        }
        else if (mGameState == GAME_STATE_STORY_CHAPTER_END)
        {
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    mCubeWrappers[i].EnableBg0SprBg1Video();
                    
                    if (i == 0)
                    {
                        DrawChapterNext(mCubeWrappers[i], mPuzzleIndex);
                    }
                    else if (i == 1)
                    {
                        DrawRetry(mCubeWrappers[i], mPuzzleIndex);
                    }
                    else if (i == 2)
                    {
                        mCubeWrappers[i].DrawBackground(ExitToMainMenu);
                    }
                }
            }
        }
    }
    else if (kGameMode == GAME_MODE_SHUFFLE)
    {
        for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
        {
            if (mCubeWrappers[i].IsEnabled())
            {
                mCubeWrappers[i].DrawBuddy();
                mCubeWrappers[i].DrawShuffleUi(
                    mGameState,
                    mScoreTimer,
                    mHintPiece0,
                    mHintPiece1);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::ShufflePieces()
{   
    ++mShuffleMoveCounter;
    
    unsigned int piece0 =
        GetRandomNonMovedPiece(mShufflePiecesMoved, arraysize(mShufflePiecesMoved));
    unsigned int piece1 =
        GetRandomOtherPiece(mShufflePiecesMoved, arraysize(mShufflePiecesMoved), piece0);
    
    OnSwapBegin(piece0, piece1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::ChooseHint()
{
    // Check all sides of all cubes against each other, looking for a perfect double swap!
    for (unsigned int iCube0 = 0; iCube0 < arraysize(mCubeWrappers); ++iCube0)
    {
        if (mCubeWrappers[iCube0].IsEnabled())
        {
            for (Cube::Side iSide0 = 0; iSide0 < NUM_SIDES; ++iSide0)
            {
                for (unsigned int iCube1 = 0; iCube1 < arraysize(mCubeWrappers); ++iCube1)
                {
                    if (mCubeWrappers[iCube1].IsEnabled() && iCube0 != iCube1)
                    {
                        for (Cube::Side iSide1 = 0; iSide1 < NUM_SIDES; ++iSide1)
                        {
                            const Piece &piece0 = mCubeWrappers[iCube0].GetPiece(iSide0);
                            const Piece &piece1 = mCubeWrappers[iCube1].GetPiece(iSide1);
                            
                            if (piece0.mMustSolve && piece0.mAttribute != Piece::ATTR_FIXED &&
                                piece1.mMustSolve && piece1.mAttribute != Piece::ATTR_FIXED)
                            {
                                const Piece &pieceSolution0 =
                                    mCubeWrappers[iCube0].GetPieceSolution(iSide0);
                                const Piece &pieceSolution1 =
                                    mCubeWrappers[iCube1].GetPieceSolution(iSide1);
                                
                                if (!Compare(piece0, pieceSolution0) &&
                                    !Compare(piece1, pieceSolution1))
                                {
                                    if (Compare(piece0, pieceSolution1) &&
                                        Compare(piece1, pieceSolution0))
                                    {
                                        mHintPiece0 = iCube0 * NUM_SIDES + iSide0;
                                        mHintPiece1 = iCube1 * NUM_SIDES + iSide1;
                                        return;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // If there are no double swaps, look for a good single swap.
    for (unsigned int iCube0 = 0; iCube0 < arraysize(mCubeWrappers); ++iCube0)
    {
        if (mCubeWrappers[iCube0].IsEnabled())
        {
            for (Cube::Side iSide0 = 0; iSide0 < NUM_SIDES; ++iSide0)
            {
                const Piece &piece0 = mCubeWrappers[iCube0].GetPiece(iSide0);
                const Piece &pieceSolution0 = mCubeWrappers[iCube0].GetPieceSolution(iSide0);
                
                if (piece0.mAttribute != Piece::ATTR_FIXED && !Compare(piece0, pieceSolution0))
                {
                    for (unsigned int iCube1 = 0; iCube1 < arraysize(mCubeWrappers); ++iCube1)
                    {
                        if (mCubeWrappers[iCube1].IsEnabled() && iCube0 != iCube1)
                        {
                            for (Cube::Side iSide1 = 0; iSide1 < NUM_SIDES; ++iSide1)
                            {
                                if (mHintPieceSkip != int(iCube1 * NUM_SIDES + iSide1))
                                {
                                    const Piece &pieceSolution1 =
                                        mCubeWrappers[iCube1].GetPieceSolution(iSide1);
                                    
                                    if (Compare(piece0, pieceSolution1))
                                    {
                                        mHintPiece0 = iCube0 * NUM_SIDES + iSide0;
                                        mHintPiece1 = iCube1 * NUM_SIDES + iSide1;
                                        return;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Clear our skip piece, see note below...
    mHintPieceSkip = -1;
    
    // If there are no single swaps, just move any piece that isn't positioned correctly.
    for (unsigned int iCube0 = 0; iCube0 < arraysize(mCubeWrappers); ++iCube0)
    {
        if (mCubeWrappers[iCube0].IsEnabled())
        {
            for (Cube::Side iSide0 = 0; iSide0 < NUM_SIDES; ++iSide0)
            {
                const Piece &piece0 = mCubeWrappers[iCube0].GetPiece(iSide0);
                const Piece &pieceSolution0 = mCubeWrappers[iCube0].GetPieceSolution(iSide0);
                
                if (piece0.mAttribute != Piece::ATTR_FIXED && !Compare(piece0, pieceSolution0))
                {
                    // Once we found a piece that's not in the correct spot, we know that there
                    // isn't a "right" place to swap it this turn (since the above loop failed)
                    // so just swap it anywhere. But make sure to remember where we swapped it
                    // so we don't just swap it back here next turn.
                    
                    Cube::ID iCube1 = (iCube0 + 1) % kNumCubes;
                    Cube::Side iSide1 = iSide0;
                    
                    mHintPiece0 = iCube0 * NUM_SIDES + iSide0;
                    mHintPiece1 = iCube1 * NUM_SIDES + iSide1;
                    
                    mHintPieceSkip = iCube1 * NUM_SIDES + iSide1;
                    return;
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TODO: Fix repeated code
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
        
        mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPieceOffset(
            mSwapPiece0 % NUM_SIDES, -kSwapAnimationCount + mSwapAnimationCounter);
        mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPieceOffset(
            mSwapPiece1 % NUM_SIDES, -kSwapAnimationCount + mSwapAnimationCounter);
        
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
        
        mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPieceOffset(
            mSwapPiece0 % NUM_SIDES, -mSwapAnimationCounter);
        mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPieceOffset(
            mSwapPiece1 % NUM_SIDES, -mSwapAnimationCounter);
        
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
    
    if (mGameState == GAME_STATE_SHUFFLE_SCRAMBLING)
    {
        bool done = GetNumMovedPieces(mShufflePiecesMoved, arraysize(mShufflePiecesMoved)) == arraysize(mShufflePiecesMoved);
        if (kShuffleMaxMoves > 0)
        {
            done = done || mShuffleMoveCounter == kShuffleMaxMoves;
        }
    
        if (done)
        {
            StartGameState(GAME_STATE_SHUFFLE_UNSCRAMBLE_THE_FACES);
        }
        else
        {
            mDelayTimer += kShuffleScrambleTimerDelay;
        }
    }
    else if (mGameState == GAME_STATE_SHUFFLE_PLAY)
    {
        if (AllSolved(*this))
        {
            StartGameState(GAME_STATE_SHUFFLE_SOLVED);
            mDelayTimer = kStateTimeDelayShort;
        }
    }
    else if (mGameState == GAME_STATE_STORY_PLAY)
    {
        if (AllSolved(*this))
        {
            StartGameState(GAME_STATE_STORY_SOLVED);
            mDelayTimer = kStateTimeDelayShort;
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
