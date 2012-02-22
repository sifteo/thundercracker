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

int AnyTouching(App& app)
{
    for (unsigned int i = 0; i < kNumCubes; ++i)
    {
        if (app.GetCubeWrapper(i).IsEnabled() && app.GetCubeWrapper(i).IsTouching())
        {
            return i;
        }
    }
    
    return -1;
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

bool NeedPaintSync(App& app)
{
    for (unsigned int i = 0; i < kNumCubes; ++i)
    {
        if (app.GetCubeWrapper(i).IsEnabled() && !app.GetCubeWrapper(i).DrawNeedsSync())
        {
            return true;
        }
    }
    
    return false;
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
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawScoreBanner(CubeWrapper &cubeWrapper, int minutes, int seconds)
{
    cubeWrapper.DrawUiAsset(
        Vec2(0, 0),
        cubeWrapper.GetId() == 0 ? ScoreTimeBlue : ScoreTimeOrange); // Banner Background
    
    const AssetImage &font = cubeWrapper.GetId() == 0 ? FontScoreBlue : FontScoreOrange;
    
    int x = 11;
    int y = 0;
    cubeWrapper.DrawUiAsset(Vec2(x++, y), font, minutes / 10); // Mintues (10s)
    cubeWrapper.DrawUiAsset(Vec2(x++, y), font, minutes % 10); // Minutes ( 1s)
    cubeWrapper.DrawUiAsset(Vec2(x++, y), font, 10); // ":"
    cubeWrapper.DrawUiAsset(Vec2(x++, y), font, seconds / 10); // Seconds (10s)
    cubeWrapper.DrawUiAsset(Vec2(x++, y), font, seconds % 10); // Seconds ( 1s)
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawChapterTitle(CubeWrapper &cubeWrapper, unsigned int puzzleIndex)
{
    String<128> buffer;
    buffer << "Chapter " << (puzzleIndex + 1) << "\n" << "\"" << GetPuzzle(puzzleIndex).GetChapterTitle() << "\"";
    
    cubeWrapper.DrawBackground(ChapterTitle);
    cubeWrapper.DrawUiText(Vec2(2, 4), buffer.c_str());
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
    
    cubeWrapper.DrawBackground(ChapterSummary);
    cubeWrapper.DrawUiText(Vec2(2, 4), buffer.c_str());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawChapterNext(CubeWrapper &cubeWrapper, unsigned int puzzleIndex)
{
    unsigned int nextPuzzleIndex = ++puzzleIndex % GetNumPuzzles();
    
    String<128> buffer;
    buffer << "Chapter " << (nextPuzzleIndex + 1);
    
    cubeWrapper.DrawBackground(ChapterNext);
    cubeWrapper.DrawUiText(Vec2(3, 8), buffer.c_str());
}
                    
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawChapterRetry(CubeWrapper &cubeWrapper, unsigned int puzzleIndex)
{
    String<128> buffer;
    buffer << "Chapter " << (puzzleIndex + 1);
    
    cubeWrapper.DrawBackground(ChapterRetry);
    cubeWrapper.DrawUiText(Vec2(3, 8), buffer.c_str());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool UpdateTimer(float &timer, float dt)
{
    ASSERT(timer > 0.0f);
    timer -= dt;
    
    if (timer <= 0.0f)
    {
        timer = 0.0f;
        return true;
    }
    else
    {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool UpdateTimerLoop(float &timer, float dt, float duration)
{
    ASSERT(timer > 0.0f);
    timer -= dt;
    
    if (timer <= 0.0f)
    {
        timer += duration;
        return true;
    }
    else
    {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool UpdateCounter(int &counter, int speed)
{
    ASSERT(counter > 0);
    counter -= speed;
    
    if (counter <= 0)
    {
        counter = 0;
        return true;
    }
    else
    {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool IsHinting(Cube::ID cubeId, int hintPiece)
{
    return hintPiece != -1 && (hintPiece / NUM_SIDES) == int(cubeId);
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
    , mShuffleMoveCounter(0)
    , mShuffleHintTimer(0.0f)
    , mPuzzleIndex(0)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::Init()
{
    // Note: manually enabling all cubes until the found/lost events start working.
    for (unsigned int i = 0; i < kNumCubes; ++i)
    {
        ASSERT(i < arraysize(mCubeWrappers));
        ASSERT(!mCubeWrappers[i].IsEnabled());
        mCubeWrappers[i].Enable(i, i % kMaxBuddies);
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
    UpdateCubes(dt);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::Draw()
{
    DrawGameState();
    
    if (NeedPaintSync(*this))
    {
        System::paintSync();
    }
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
    else if (mGameState == GAME_STATE_STORY_HINT_1)
    {
        StartGameState(GAME_STATE_STORY_PLAY);
    }
    else if (mGameState == GAME_STATE_STORY_HINT_2)
    {
        ASSERT(mHintPiece0 != -1);
        mCubeWrappers[mHintPiece0 / NUM_SIDES].StopPieceBlinking();
        
        ASSERT(mHintPiece1 != -1);
        mCubeWrappers[mHintPiece1 / NUM_SIDES].StopPieceBlinking();
        
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
    
    // TODO: Put this into the correct cases
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
    else if (mGameState == GAME_STATE_STORY_HINT_1)
    {
        StartGameState(GAME_STATE_STORY_PLAY);
    }
    else if (mGameState == GAME_STATE_STORY_HINT_2)
    {
        ASSERT(mHintPiece0 != -1);
        mCubeWrappers[mHintPiece0 / NUM_SIDES].StopPieceBlinking();
        
        ASSERT(mHintPiece1 != -1);
        mCubeWrappers[mHintPiece1 / NUM_SIDES].StopPieceBlinking();
        
        StartGameState(GAME_STATE_STORY_PLAY);
    }
    
    // TODO: Put this into the correct cases
    mShuffleHintTimer = kHintTimerOnDuration;
    mHintPiece0 = -1;
    mHintPiece1 = -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::OnShake(Cube::ID cubeId)
{
    if(mGameState == GAME_STATE_SHUFFLE_SHAKE_TO_SCRAMBLE)
    {
        StartGameState(GAME_STATE_SHUFFLE_SCRAMBLING);
    }
    else if (mGameState == GAME_STATE_SHUFFLE_SCORE)
    {
        StartGameState(GAME_STATE_SHUFFLE_SCRAMBLING);
    }
    else if (mGameState == GAME_STATE_STORY_CLUE)
    {
        StartGameState(GAME_STATE_STORY_PLAY);
    }
    else if (mGameState == GAME_STATE_STORY_HINT_1)
    {
        StartGameState(GAME_STATE_STORY_PLAY);
    }
    else if (mGameState == GAME_STATE_STORY_HINT_2)
    {
        ASSERT(mHintPiece0 != -1);
        mCubeWrappers[mHintPiece0 / NUM_SIDES].StopPieceBlinking();
        
        ASSERT(mHintPiece1 != -1);
        mCubeWrappers[mHintPiece1 / NUM_SIDES].StopPieceBlinking();
        
        StartGameState(GAME_STATE_STORY_PLAY);
    }
    
    // TODO: Put this into the correct cases
    mShuffleHintTimer = kHintTimerOnDuration;
    mHintPiece0 = -1;
    mHintPiece1 = -1;
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

void App::UpdateCubes(float dt)
{
    for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
    {
        if (mCubeWrappers[i].IsEnabled())
        {
            mCubeWrappers[i].Update(dt);
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
            mCubeWrappers[mHintPiece0 / NUM_SIDES].StartPieceBlinking(mHintPiece0 % NUM_SIDES);
            mCubeWrappers[mHintPiece1 / NUM_SIDES].StartPieceBlinking(mHintPiece1 % NUM_SIDES);
            
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
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::UpdateGameState(float dt)
{
    switch (mGameState)
    {
        case GAME_STATE_SHUFFLE_START:
        {
            if (UpdateTimer(mDelayTimer, dt))
            {
                StartGameState(GAME_STATE_SHUFFLE_SHAKE_TO_SCRAMBLE);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_SCRAMBLING:
        {
            if (mSwapAnimationCounter == 0)
            {
                if (UpdateTimer(mDelayTimer, dt))
                {
                    ShufflePieces();
                }
            }
            break;
        }
        case GAME_STATE_SHUFFLE_UNSCRAMBLE_THE_FACES:
        {
            if (OnTouch() == TOUCH_EVENT_BEGIN)
            {
                StartGameState(GAME_STATE_SHUFFLE_PLAY);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_PLAY:
        {
            mScoreTimer += dt;
            
            if (mShuffleHintTimer > 0.0f)
            {
                if (UpdateTimer(mShuffleHintTimer, dt))
                {
                    ChooseHint();
                }
            }
            
            TouchEvent touch = OnTouch();
            if (touch == TOUCH_EVENT_BEGIN)
            {
                ChooseHint();
            }
            else if (touch == TOUCH_EVENT_END)
            {
                mShuffleHintTimer = kHintTimerOnDuration;
                mHintPiece0 = -1;
                mHintPiece1 = -1;
            }
            break;
        }
        case GAME_STATE_SHUFFLE_SOLVED:
        {
            if (UpdateTimer(mDelayTimer, dt))
            {
                StartGameState(GAME_STATE_SHUFFLE_SCORE);
            }
            break;
        }
        case GAME_STATE_STORY_CHAPTER_START:
        {
            if (UpdateTimer(mDelayTimer, dt))
            {
                StartGameState(GAME_STATE_STORY_CUTSCENE_START);
            }
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_START:
        {
            mCubeWrappers[0].UpdateCutscene();
            
            if (UpdateTimer(mDelayTimer, dt))
            {
                StartGameState(GAME_STATE_STORY_DISPLAY_START_STATE);
            }
            break;
        }
        case GAME_STATE_STORY_DISPLAY_START_STATE:
        {
            if (UpdateTimer(mDelayTimer, dt))
            {
                StartGameState(GAME_STATE_STORY_CLUE);
            }
            break;
        }
        case GAME_STATE_STORY_CLUE:
        {
            if (OnTouch() == TOUCH_EVENT_BEGIN)
            {
                StartGameState(GAME_STATE_STORY_PLAY);
            }
            break;
        }
        case GAME_STATE_STORY_PLAY:
        {
            mScoreTimer += dt;
            
            if (OnTouch() == TOUCH_EVENT_BEGIN)
            {
                StartGameState(GAME_STATE_STORY_HINT_1);
            }
            break;
        }
        case GAME_STATE_STORY_HINT_1:
        {
            mScoreTimer += dt;
            
            if (UpdateTimer(mDelayTimer, dt))
            {
                StartGameState(GAME_STATE_STORY_PLAY);
            }
            
            if (OnTouch() == TOUCH_EVENT_BEGIN)
            {
                StartGameState(GAME_STATE_STORY_HINT_2);
            }
            break;
        }
        case GAME_STATE_STORY_HINT_2:
        {
            mScoreTimer += dt;
            
            if (OnTouch() == TOUCH_EVENT_BEGIN)
            {
                ASSERT(mHintPiece0 != -1);
                mCubeWrappers[mHintPiece0 / NUM_SIDES].StopPieceBlinking();
                
                ASSERT(mHintPiece1 != -1);
                mCubeWrappers[mHintPiece1 / NUM_SIDES].StopPieceBlinking();
                
                StartGameState(GAME_STATE_STORY_PLAY);
            }
            break;
        }
        case GAME_STATE_STORY_SOLVED:
        {
            if (UpdateTimer(mDelayTimer, dt))
            {
                StartGameState(GAME_STATE_STORY_CUTSCENE_END);
            }
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_END:
        {
            mCubeWrappers[0].UpdateCutscene();
            
            if (UpdateTimer(mDelayTimer, dt))
            {
                StartGameState(GAME_STATE_STORY_CHAPTER_END);
            }
            break;
        }
        case GAME_STATE_STORY_CHAPTER_END:
        {
            Cube::ID cubeId;
            if (OnTouch(&cubeId))
            {
                if (cubeId == 0)
                {
                    mPuzzleIndex = (mPuzzleIndex + 1) % GetNumPuzzles();
                    StartGameState(GAME_STATE_STORY_CHAPTER_START);
                }
                else if (cubeId == 1)
                {
                    StartGameState(GAME_STATE_STORY_CHAPTER_START);
                }
                else if (cubeId == 2)
                {
                    // Go to main menu...
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

void App::DrawGameState()
{
    for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
    {
        if (mCubeWrappers[i].IsEnabled())
        {
            mCubeWrappers[i].DrawClear();
            DrawGameStateCube(mCubeWrappers[i]);
            mCubeWrappers[i].DrawFlush();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::DrawGameStateCube(CubeWrapper &cubeWrapper)
{
    switch (mGameState)
    {
        case GAME_STATE_FREE_PLAY:
        {
            cubeWrapper.DrawBuddy();
            break;
        }
        case GAME_STATE_SHUFFLE_START:
        {
            cubeWrapper.DrawBuddy();
            break;
        }
        case GAME_STATE_SHUFFLE_SHAKE_TO_SCRAMBLE:
        {
            cubeWrapper.DrawBuddy();
            cubeWrapper.DrawUiAsset(
                Vec2(0, 0),
                cubeWrapper.GetId() == 0 ? ShakeToShuffleBlue :  ShakeToShuffleOrange);
            break;
        }
        case GAME_STATE_SHUFFLE_SCRAMBLING:
        {
            cubeWrapper.DrawBuddy();
            break;
        }
        case GAME_STATE_SHUFFLE_UNSCRAMBLE_THE_FACES:
        {
            cubeWrapper.DrawBuddy();
            cubeWrapper.DrawUiAsset(
                Vec2(0, 0),
                cubeWrapper.GetId() ? UnscrableTheFacesBlue : UnscrableTheFacesOrange);
            break;
        }
        case GAME_STATE_SHUFFLE_PLAY:
        {
            cubeWrapper.DrawBuddy();
            
            if (IsHinting(cubeWrapper.GetId(), mHintPiece0))
            {      
                cubeWrapper.DrawUiAsset(
                    GetHintBarPoint(mHintPiece0 % NUM_SIDES),
                    GetHintBarAsset(cubeWrapper.GetId(), mHintPiece0 % NUM_SIDES));
            }
            else if (IsHinting(cubeWrapper.GetId(), mHintPiece1))
            {
                cubeWrapper.DrawUiAsset(
                    GetHintBarPoint(mHintPiece1 % NUM_SIDES),
                    GetHintBarAsset(cubeWrapper.GetId(), mHintPiece1 % NUM_SIDES));
            }
            else if (cubeWrapper.IsSolved())
            {
                cubeWrapper.DrawUiAsset(
                    Vec2(0, 0),
                    cubeWrapper.GetId() == 0 ? FaceCompleteBlue : FaceCompleteOrange);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_SOLVED:
        {
            cubeWrapper.DrawBuddy();
            break;
        }
        case GAME_STATE_SHUFFLE_SCORE:
        {
            cubeWrapper.DrawBuddy();
            
            if (cubeWrapper.GetId() == 0)
            {
                int minutes = int(mScoreTimer) / 60;
                int seconds = int(mScoreTimer - (minutes * 60.0f));
                DrawScoreBanner(cubeWrapper, minutes, seconds);
            }
            else
            {
                cubeWrapper.DrawUiAsset(Vec2(0, 0), ShakeToShuffleOrange);
            }
            break;
        }
        case GAME_STATE_STORY_CHAPTER_START:
        {
            DrawChapterTitle(cubeWrapper, mPuzzleIndex);
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_START:
        {
            if (cubeWrapper.GetId() == 0)
            {
                cubeWrapper.DrawCutscene(GetPuzzle(mPuzzleIndex).GetCutsceneTextStart());
            }
            else
            {
                DrawChapterTitle(cubeWrapper, mPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_DISPLAY_START_STATE:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mPuzzleIndex).GetNumBuddies())
            {
                cubeWrapper.DrawBuddy();
            }
            else
            {
                DrawChapterTitle(cubeWrapper, mPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_CLUE:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mPuzzleIndex).GetNumBuddies())
            {
                cubeWrapper.DrawBuddy();
                cubeWrapper.DrawUiAsset(Vec2(0, 3), ClueText);
                cubeWrapper.DrawUiText(Vec2(2, 4), GetPuzzle(mPuzzleIndex).GetClue());
            }
            else
            {
                DrawChapterTitle(cubeWrapper, mPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_PLAY:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mPuzzleIndex).GetNumBuddies())
            {
                cubeWrapper.DrawBuddy();
            }
            else
            {
                DrawChapterTitle(cubeWrapper, mPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_HINT_1:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mPuzzleIndex).GetNumBuddies())
            {
                cubeWrapper.DrawBuddy();
                
                if (cubeWrapper.GetId() == 0)
                {
                    cubeWrapper.DrawUiAsset(Vec2(0, 3), MoreHints);
                    cubeWrapper.DrawUiText(Vec2(2, 4), GetPuzzle(mPuzzleIndex).GetClue());
                }
            }
            else
            {
                DrawChapterTitle(cubeWrapper, mPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_HINT_2:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mPuzzleIndex).GetNumBuddies())
            {
                cubeWrapper.DrawBuddy();
            }
            else
            {
                DrawChapterTitle(cubeWrapper, mPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_SOLVED:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mPuzzleIndex).GetNumBuddies())
            {
                cubeWrapper.DrawBuddy();
            }
            else
            {
                DrawChapterTitle(cubeWrapper, mPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_END:
        {
            if (cubeWrapper.GetId() == 0)
            {
                cubeWrapper.DrawCutscene(GetPuzzle(mPuzzleIndex).GetCutsceneTextEnd());
            }
            else
            {
                DrawChapterSummary(cubeWrapper, mPuzzleIndex, mScoreTimer, mScoreMoves);
            }
            break;
        }
        case GAME_STATE_STORY_CHAPTER_END:
        {
            if (cubeWrapper.GetId() == 0)
            {
                DrawChapterNext(cubeWrapper, mPuzzleIndex);
            }
            else if (cubeWrapper.GetId() == 1)
            {
                DrawChapterRetry(cubeWrapper, mPuzzleIndex);
            }
            else if (cubeWrapper.GetId() == 2)
            {
                cubeWrapper.DrawBackground(ExitToMainMenu);
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
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::UpdateSwap(float dt)
{
    if (mSwapState == SWAP_STATE_OUT)
    {
        bool done = UpdateCounter(mSwapAnimationCounter, kSwapAnimationSpeed);
        
        mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPieceOffset(
            mSwapPiece0 % NUM_SIDES, -kSwapAnimationCount + mSwapAnimationCounter);
        mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPieceOffset(
            mSwapPiece1 % NUM_SIDES, -kSwapAnimationCount + mSwapAnimationCounter);
        
        if (done)
        {
            OnSwapExchange();
        }
    }
    else if (mSwapState == SWAP_STATE_IN)
    {
        bool done = UpdateCounter(mSwapAnimationCounter, kSwapAnimationSpeed);
        
        mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPieceOffset(
            mSwapPiece0 % NUM_SIDES, -mSwapAnimationCounter);
        mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPieceOffset(
            mSwapPiece1 % NUM_SIDES, -mSwapAnimationCounter);
        
        if (done)
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
        bool done = GetNumMovedPieces(
            mShufflePiecesMoved, arraysize(mShufflePiecesMoved)) == arraysize(mShufflePiecesMoved);
        
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

App::TouchEvent App::OnTouch(Sifteo::Cube::ID *cubeId)
{
    if (!mTouching)
    {
        int touching = AnyTouching(*this);
        if (touching != -1)
        {
            if (cubeId != NULL)
            {
                *cubeId = touching;
            }
            mTouching = true;
            return TOUCH_EVENT_BEGIN;
        }
    }
    else
    {
        if (AnyTouching(*this) == -1)
        {
            mTouching = false;
            return TOUCH_EVENT_END;
        }
    }
    
    return TOUCH_EVENT_NONE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}
