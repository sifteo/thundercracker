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
        if (app.GetCubeWrapper(i).IsEnabled() && app.GetCubeWrapper(i).DrawNeedsSync())
        {
            return true;
        }
    }
    
    return false;
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
    buffer << "Chapter " << (puzzleIndex + 1) << "\n" << "\"" << GetPuzzle(puzzleIndex).GetTitle() << "\"";
    
    cubeWrapper.DrawBackground(ChapterTitle);
    cubeWrapper.DrawUiText(Vec2(1, 6), FontOrange, buffer.c_str());
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
    
    cubeWrapper.DrawBackground(ChapterTitle);
    cubeWrapper.DrawUiText(Vec2(1, 6), FontOrange, buffer.c_str());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawChapterNext(CubeWrapper &cubeWrapper, unsigned int puzzleIndex)
{
    unsigned int nextPuzzleIndex = ++puzzleIndex % GetNumPuzzles();
    
    String<128> buffer;
    buffer << "Chapter " << (nextPuzzleIndex + 1);
    
    cubeWrapper.DrawBackground(ChapterNext);
    cubeWrapper.DrawUiText(Vec2(3, 8), FontOrange, buffer.c_str());
}
                    
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawChapterRetry(CubeWrapper &cubeWrapper, unsigned int puzzleIndex)
{
    String<128> buffer;
    buffer << "Chapter " << (puzzleIndex + 1);
    
    cubeWrapper.DrawBackground(ChapterRetry);
    cubeWrapper.DrawUiText(Vec2(3, 8), FontOrange, buffer.c_str());
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

bool IsBuddyUsed(App &app, unsigned buddyId)
{
    for (unsigned int i = 0; i < kNumCubes; ++i)
    {
        if (app.GetCubeWrapper(i).IsEnabled() &&
            app.GetCubeWrapper(i).GetBuddyId() == buddyId)
        {
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool IsAtRotationTarget(const Piece &piece, Cube::Side side)
{
    int rotation = side - piece.GetPart();
    if (rotation < 0)
    {
        rotation += NUM_SIDES;
    }
    
    return piece.GetRotation() == rotation;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

int ClampMod(int value, int modulus)
{
    while (value < 0)
    {
        value += modulus;
    }
    
    while (value >= modulus)
    {
        value -= modulus;
    }
    
    return value;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

const AssetImage &GetBuddyFullAsset(int buddyId)
{
    switch (buddyId)
    {
        default:
        case 0: return BuddyFull0;
        case 1: return BuddyFull1;
        case 2: return BuddyFull2;
        case 3: return BuddyFull3;
        case 4: return BuddyFull4;
        case 5: return BuddyFull5;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

const char *kGameStateNames[NUM_GAME_STATES] =
{
    "GAME_STATE_NONE",
    "GAME_STATE_MAIN_MENU",
    "GAME_STATE_FREE_PLAY",
    "GAME_STATE_SHUFFLE_START",
    "GAME_STATE_SHUFFLE_TITLE",
    "GAME_STATE_SHUFFLE_CHARACTER_SPLASH",
    "GAME_STATE_SHUFFLE_SHAKE_TO_SHUFFLE",
    "GAME_STATE_SHUFFLE_SHUFFLING",
    "GAME_STATE_SHUFFLE_UNSHUFFLE_THE_FACES",
    "GAME_STATE_SHUFFLE_PLAY",
    "GAME_STATE_SHUFFLE_HINT",
    "GAME_STATE_SHUFFLE_SOLVED",
    "GAME_STATE_SHUFFLE_CONGRATULATIONS",
    "GAME_STATE_SHUFFLE_SCORE",
    "GAME_STATE_STORY_START",
    "GAME_STATE_STORY_CHAPTER_START",
    "GAME_STATE_STORY_CUTSCENE_START",
    "GAME_STATE_STORY_DISPLAY_START_STATE",
    "GAME_STATE_STORY_SCRAMBLING",
    "GAME_STATE_STORY_CLUE",
    "GAME_STATE_STORY_PLAY",
    "GAME_STATE_STORY_HINT_CLUE",
    "GAME_STATE_STORY_HINT_MOVE",
    "GAME_STATE_STORY_SOLVED",
    "GAME_STATE_STORY_CUTSCENE_END",
    "GAME_STATE_STORY_CHAPTER_END",
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
    , mTouching()
    , mScoreTimer(0.0f)
    , mScoreMoves(0)
    , mSwapState(SWAP_STATE_NONE)
    , mSwapPiece0(0)
    , mSwapPiece1(0)
    , mSwapAnimationSlideTimer(0)
    , mSwapAnimationRotateTimer(0.0f)
    , mFaceCompleteTimers()
    , mHintTimer(0.0f)
    , mHintPiece0(-1)
    , mHintPiece1(-1)
    , mHintPieceSkip(-1)
    , mHintCubeTouched(CUBE_ID_UNDEFINED)
    , mFreePlayShakeThrottleTimer(0.0f)
    , mShuffleUiIndex(0)
    , mShuffleMoveCounter(0)
    , mStoryPuzzleIndex(0)
    , mStoryCutsceneIndex(0)
{
    for (unsigned int i = 0; i < arraysize(mTouching); ++i)
    {
        mTouching[i] = TOUCH_STATE_NONE;
    }
    
    for (unsigned int i = 0; i < arraysize(mFaceCompleteTimers); ++i)
    {
        mFaceCompleteTimers[i] = 0.0f;
    }
    
    for (unsigned int i = 0; i < arraysize(mShufflePiecesMoved); ++i)
    {
        mShufflePiecesMoved[i] = false;
    }
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
        mCubeWrappers[i].Enable(i);
    }
    
    InitializePuzzles();
    
#ifdef SIFTEO_SIMULATOR
    mChannel.init();
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::Reset()
{
    StartGameState(kStateDefault);
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
    if (mGameState == GAME_STATE_MAIN_MENU)
    {
        if (cubeId0 == 0 && cubeId1 == 1 && cubeSide1 == SIDE_TOP)
        {
            switch (cubeSide0)
            {
                case SIDE_TOP:
                {
                    StartGameState(GAME_STATE_FREE_PLAY);
                    break;
                }
                case SIDE_LEFT:
                {
                    StartGameState(GAME_STATE_SHUFFLE_START);
                    break;
                }
                case SIDE_BOTTOM:
                {
                    StartGameState(GAME_STATE_STORY_START);
                    break;
                }
                case SIDE_RIGHT:
                {
                    break;
                }
            }
        }
        else if (cubeId1 == 0 && cubeId0 == 1 && cubeSide0 == SIDE_TOP)
        {
            switch (cubeSide1)
            {
                case SIDE_TOP:
                {
                    StartGameState(GAME_STATE_FREE_PLAY);
                    break;
                }
                case SIDE_LEFT:
                {
                    StartGameState(GAME_STATE_SHUFFLE_START);
                    break;
                }
                case SIDE_BOTTOM:
                {
                    StartGameState(GAME_STATE_STORY_START);
                    break;
                }
                case SIDE_RIGHT:
                {
                    break;
                }
            }
        }
    }
    else if (mGameState == GAME_STATE_STORY_CLUE)
    {
        StartGameState(GAME_STATE_STORY_PLAY);
    }
    else if (mGameState == GAME_STATE_STORY_HINT_CLUE)
    {
        mHintCubeTouched = CUBE_ID_UNDEFINED;
        StartGameState(GAME_STATE_STORY_PLAY);
    }
    else
    {
        if (mGameState == GAME_STATE_SHUFFLE_UNSHUFFLE_THE_FACES)
        {
            StartGameState(GAME_STATE_SHUFFLE_PLAY);
        }
        else if (mGameState == GAME_STATE_SHUFFLE_HINT)
        {
            StopHint();
            StartGameState(GAME_STATE_SHUFFLE_PLAY);
        }
        else if (mGameState == GAME_STATE_STORY_HINT_MOVE)
        {
            StopHint();
            StartGameState(GAME_STATE_STORY_PLAY);
        }
        
        bool isSwapping = mSwapState != SWAP_STATE_NONE;
        
        bool isFixed =
            GetCubeWrapper(cubeId0).GetPiece(cubeSide0).GetAttribute() == Piece::ATTR_FIXED ||
            GetCubeWrapper(cubeId1).GetPiece(cubeSide1).GetAttribute() == Piece::ATTR_FIXED;
        
        bool isValidGameState =
            mGameState == GAME_STATE_FREE_PLAY ||
            mGameState == GAME_STATE_SHUFFLE_PLAY ||
            mGameState == GAME_STATE_STORY_PLAY;
        
        bool isValidCube =
            mGameState != GAME_STATE_STORY_PLAY ||
            (   cubeId0 < GetPuzzle(mStoryPuzzleIndex).GetNumBuddies() &&
                cubeId1 < GetPuzzle(mStoryPuzzleIndex).GetNumBuddies());
        
        if (!isSwapping && !isFixed && isValidGameState && isValidCube)
        {
            ++mScoreMoves;
            OnSwapBegin(cubeId0 * NUM_SIDES + cubeSide0, cubeId1 * NUM_SIDES + cubeSide1);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::OnTilt(Cube::ID cubeId)
{
    if (mGameState == GAME_STATE_FREE_PLAY)
    {
        if (mSwapState == SWAP_STATE_NONE)
        {
            ASSERT(cubeId < arraysize(mCubeWrappers));
            Cube::TiltState tiltState = mCubeWrappers[cubeId].GetTiltState();
            
            mCubeWrappers[cubeId].SetPieceOffset(
                SIDE_TOP,
                Vec2((tiltState.x - 1) * VidMode::TILE, (tiltState.y - 1) * VidMode::TILE));
            mCubeWrappers[cubeId].SetPieceOffset(
                SIDE_LEFT,
                Vec2((tiltState.x - 1) * VidMode::TILE, (tiltState.y - 1) * VidMode::TILE));
            mCubeWrappers[cubeId].SetPieceOffset(
                SIDE_BOTTOM,
                Vec2(-(tiltState.x - 1) * VidMode::TILE, -(tiltState.y - 1) * VidMode::TILE));
            mCubeWrappers[cubeId].SetPieceOffset(
                SIDE_RIGHT,
                Vec2(-(tiltState.x - 1) * VidMode::TILE, (tiltState.y - 1) * VidMode::TILE));
        }
    }
    else if (mGameState == GAME_STATE_SHUFFLE_UNSHUFFLE_THE_FACES)
    {
        StartGameState(GAME_STATE_SHUFFLE_PLAY);
    }
    else if (mGameState == GAME_STATE_SHUFFLE_HINT)
    {
        StopHint();
        StartGameState(GAME_STATE_SHUFFLE_PLAY);
    }
    else if (mGameState == GAME_STATE_STORY_CLUE)
    {
        StartGameState(GAME_STATE_STORY_PLAY);
    }
    else if (mGameState == GAME_STATE_STORY_HINT_CLUE)
    {
        mHintCubeTouched = CUBE_ID_UNDEFINED;
        StartGameState(GAME_STATE_STORY_PLAY);
    }
    else if (mGameState == GAME_STATE_STORY_HINT_MOVE)
    {
        StopHint();
        StartGameState(GAME_STATE_STORY_PLAY);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::OnShake(Cube::ID cubeId)
{
    if (mGameState == GAME_STATE_FREE_PLAY)
    {
        if (mSwapState == SWAP_STATE_NONE && mFreePlayShakeThrottleTimer == 0.0f)
        {
            mFreePlayShakeThrottleTimer = kFreePlayShakeThrottleDuration;
        
            Random random;
            int selection = random.randrange(kMaxBuddies - kNumCubes);
            
            unsigned buddyId = mCubeWrappers[cubeId].GetBuddyId();
            for (int i = 0; i < selection; ++i)
            {
                buddyId = (buddyId + 1) % kMaxBuddies;
                while (IsBuddyUsed(*this, buddyId))
                {
                    buddyId = (buddyId + 1) % kMaxBuddies;
                }
            }
            mCubeWrappers[cubeId].SetBuddyId(buddyId);
            
            ResetCubesToPuzzle(GetPuzzleDefault(), false);
        }
    }
    else if (mGameState == GAME_STATE_SHUFFLE_SHAKE_TO_SHUFFLE)
    {
        StartGameState(GAME_STATE_SHUFFLE_SHUFFLING);
    }
    else if (mGameState == GAME_STATE_SHUFFLE_HINT)
    {
        StopHint();
        StartGameState(GAME_STATE_SHUFFLE_PLAY);
    }
    else if (mGameState == GAME_STATE_SHUFFLE_SCORE)
    {
        if (cubeId == 1)
        {
            StartGameState(GAME_STATE_SHUFFLE_SHUFFLING);
        }
    }
    else if (mGameState == GAME_STATE_STORY_CLUE)
    {
        StartGameState(GAME_STATE_STORY_PLAY);
    }
    else if (mGameState == GAME_STATE_STORY_HINT_CLUE)
    {
        mHintCubeTouched = CUBE_ID_UNDEFINED;
        StartGameState(GAME_STATE_STORY_PLAY);
    }
    else if (mGameState == GAME_STATE_STORY_HINT_MOVE)
    {
        StopHint();
        StartGameState(GAME_STATE_STORY_PLAY);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::ResetCubesToPuzzle(const Puzzle &puzzle, bool resetBuddies)
{
    for (unsigned int i = 0; i < puzzle.GetNumBuddies(); ++i)
    {
        if (i < arraysize(mCubeWrappers) && mCubeWrappers[i].IsEnabled())
        {
            mCubeWrappers[i].Reset();
            
            if (resetBuddies)
            {
                mCubeWrappers[i].SetBuddyId(puzzle.GetBuddy(i));
                
                for (unsigned int j = 0; j < NUM_SIDES; ++j)
                {
                    mCubeWrappers[i].SetPiece(
                        j, puzzle.GetPieceStart(i, j));
                    mCubeWrappers[i].SetPieceSolution(
                        j, puzzle.GetPieceEnd(i, j));
                }
            }
            else
            {
                // TODO: This is definitely hacky... instead of igoring the active puzzle,
                // the Free Play shake shuffle should alter the current Puzzle. But then we'd
                // need to keep an editable copy of Puzzle around...
                for (unsigned int j = 0; j < NUM_SIDES; ++j)
                {
                    mCubeWrappers[i].SetPiece(
                        j, puzzle.GetPieceStart(mCubeWrappers[i].GetBuddyId(), j));
                    mCubeWrappers[i].SetPieceSolution(
                        j, puzzle.GetPieceEnd(mCubeWrappers[i].GetBuddyId(), j));
                }
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
            mFreePlayShakeThrottleTimer = 0.0f;
            
            unsigned int buddyIds[kMaxBuddies];
            for (unsigned int i = 0; i < arraysize(buddyIds); ++i)
            {
                buddyIds[i] = i;
            }
            
            // Fisher-Yates Shuffle
            Random random;
            for (unsigned int i = arraysize(buddyIds) - 1; i > 0; --i)
            {
                int j = random.randrange(i + 1);
                int temp = buddyIds[j];
                buddyIds[j] = buddyIds[i];
                buddyIds[i] = temp;
            }
            
            // Assign IDs to the buddies
            for (unsigned int i = 0; i < kNumCubes; ++i)
            {
                mCubeWrappers[i].SetBuddyId(buddyIds[i]);
            }
            
            ResetCubesToPuzzle(GetPuzzleDefault(), false);
            break;
        }
        case GAME_STATE_SHUFFLE_START:
        {
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    mCubeWrappers[i].SetBuddyId(i % kMaxBuddies);
                }
            }
            ResetCubesToPuzzle(GetPuzzleDefault(), true);
            StartGameState(GAME_STATE_SHUFFLE_TITLE);
            break;
        }
        case GAME_STATE_SHUFFLE_TITLE:
        {
            mDelayTimer = kStateTimeDelayLong;
            break;
        }
        case GAME_STATE_SHUFFLE_CHARACTER_SPLASH:
        {
            mDelayTimer = kStateTimeDelayLong;
            break;
        }
        case GAME_STATE_SHUFFLE_SHAKE_TO_SHUFFLE:
        {
            mShuffleUiIndex = 0;
            mDelayTimer = kStateTimeDelayLong;
            break;   
        }
        case GAME_STATE_SHUFFLE_SHUFFLING:
        {
            mShuffleMoveCounter = 0;
            for (unsigned int i = 0; i < arraysize(mShufflePiecesMoved); ++i)
            {
                mShufflePiecesMoved[i] = false;
            }
            ShufflePieces(kNumCubes);
            break;
        }
        case GAME_STATE_SHUFFLE_UNSHUFFLE_THE_FACES:
        {
            mScoreTimer = 0.0f;
            mScoreMoves = 0;
            mDelayTimer = kStateTimeDelayLong;
            mShuffleUiIndex = 0;
            break;
        }
        case GAME_STATE_SHUFFLE_PLAY:
        {
            for (unsigned int i = 0; i < arraysize(mFaceCompleteTimers); ++i)
            {
                mFaceCompleteTimers[i] = 0.0f;
            }
            mHintTimer = kHintTimerOnDuration;
            break;
        }
        case GAME_STATE_SHUFFLE_HINT:
        {
            StartHint();
            mHintTimer = kHintTimerOffDuration;
            break;
        }
        case GAME_STATE_SHUFFLE_SOLVED:
        {
            mDelayTimer = kStateTimeDelayLong;
            break;
        }
        case GAME_STATE_SHUFFLE_CONGRATULATIONS:
        {
            mDelayTimer = kStateTimeDelayLong;
            break;
        }
        case GAME_STATE_STORY_START:
        {
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    mCubeWrappers[i].SetBuddyId(i % kMaxBuddies);
                }
            }
            mStoryPuzzleIndex = 0;
            StartGameState(GAME_STATE_STORY_CHAPTER_START);
            break;
        }
        case GAME_STATE_STORY_CHAPTER_START:
        {
            ASSERT(mStoryPuzzleIndex < GetNumPuzzles());
            ResetCubesToPuzzle(GetPuzzle(mStoryPuzzleIndex), true);
            mDelayTimer = kStateTimeDelayLong;
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_START:
        {
            if (GetPuzzle(mStoryPuzzleIndex).GetNumCutsceneTextStart() == 1)
            {
                mDelayTimer = kStateTimeDelayLong;
            }
            else
            {
                mDelayTimer = kCutsceneTextDelay;
            }
            mStoryCutsceneIndex = 0;
            break;
        }
        case GAME_STATE_STORY_DISPLAY_START_STATE:
        {
            mDelayTimer = kStateTimeDelayShort;
            break;
        }
        case GAME_STATE_STORY_SCRAMBLING:
        {
            mShuffleMoveCounter = 0;
            for (unsigned int i = 0; i < arraysize(mShufflePiecesMoved); ++i)
            {
                mShufflePiecesMoved[i] = false;
            }
            ShufflePieces(GetPuzzle(mStoryPuzzleIndex).GetNumBuddies());
            break;
        }
        case GAME_STATE_STORY_PLAY:
        {
            mScoreTimer = 0.0f;
            mScoreMoves = 0;
            for (unsigned int i = 0; i < arraysize(mFaceCompleteTimers); ++i)
            {
                mFaceCompleteTimers[i] = 0.0f;
            }
            mHintTimer = kHintTimerOnDuration;
            break;
        }
        case GAME_STATE_STORY_HINT_CLUE:
        {
            mDelayTimer = kStateTimeDelayLong;
            break;
        }
        case GAME_STATE_STORY_HINT_MOVE:
        {
            StartHint();
            mHintTimer = kHintTimerOffDuration;
            break;
        }
        case GAME_STATE_STORY_SOLVED:
        {
            mDelayTimer = kStateTimeDelayLong;
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_END:
        {
            if (GetPuzzle(mStoryPuzzleIndex).GetNumCutsceneTextEnd() == 1)
            {
                mDelayTimer = kStateTimeDelayLong;
            }
            else
            {
                mDelayTimer = kCutsceneTextDelay;
            }
            mStoryCutsceneIndex = 0;
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
    for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
    {
        if (mCubeWrappers[i].IsEnabled())
        {
            if (mTouching[i] == TOUCH_STATE_NONE)
            {
                if (mCubeWrappers[i].IsTouching())
                {
                    mTouching[i] = TOUCH_STATE_BEGIN;
                }
            }
            else if (mTouching[i] == TOUCH_STATE_BEGIN)
            {
                if (mCubeWrappers[i].IsTouching())
                {
                    mTouching[i] = TOUCH_STATE_HOLD;
                }
                else
                {
                    mTouching[i] = TOUCH_STATE_END;
                }
            }
            else if (mTouching[i] == TOUCH_STATE_HOLD)
            {
                if (!mCubeWrappers[i].IsTouching())
                {
                    mTouching[i] = TOUCH_STATE_END;
                }
            }
            else if (mTouching[i] == TOUCH_STATE_END)
            {
                if (mCubeWrappers[i].IsTouching())
                {
                    mTouching[i] = TOUCH_STATE_BEGIN;
                }
                else
                {
                    mTouching[i] = TOUCH_STATE_NONE;
                }
            }
        }
    }
    
    switch (mGameState)
    {
        case GAME_STATE_FREE_PLAY:
        {
            if (mSwapState == SWAP_STATE_NONE)
            {
                if (mFreePlayShakeThrottleTimer > 0.0f)
                {
                    UpdateTimer(mFreePlayShakeThrottleTimer, dt);
                }
                
                for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
                {
                    if (mCubeWrappers[i].IsEnabled())
                    {
                        if (mTouching[i] == TOUCH_STATE_BEGIN ||
                            mTouching[i] == TOUCH_STATE_HOLD)
                        {
                            mCubeWrappers[i].SetPieceOffset(SIDE_TOP,    Vec2(0, VidMode::TILE));
                            mCubeWrappers[i].SetPieceOffset(SIDE_LEFT,   Vec2(VidMode::TILE, 0));
                            mCubeWrappers[i].SetPieceOffset(SIDE_BOTTOM, Vec2(0, VidMode::TILE));
                            mCubeWrappers[i].SetPieceOffset(SIDE_RIGHT,  Vec2(VidMode::TILE, 0));
                        }
                        else if (mTouching[i] == TOUCH_STATE_END)
                        {
                            mCubeWrappers[i].SetPieceOffset(SIDE_TOP,    Vec2(0, 0));
                            mCubeWrappers[i].SetPieceOffset(SIDE_LEFT,   Vec2(0, 0));
                            mCubeWrappers[i].SetPieceOffset(SIDE_BOTTOM, Vec2(0, 0));
                            mCubeWrappers[i].SetPieceOffset(SIDE_RIGHT,  Vec2(0, 0));
                        }
                    }
                }
            }
            break;
        }
        case GAME_STATE_SHUFFLE_TITLE:
        {
            if (UpdateTimer(mDelayTimer, dt))
            {
                StartGameState(GAME_STATE_SHUFFLE_CHARACTER_SPLASH);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_CHARACTER_SPLASH:
        {
            if (UpdateTimer(mDelayTimer, dt))
            {
                StartGameState(GAME_STATE_SHUFFLE_SHAKE_TO_SHUFFLE);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_SHAKE_TO_SHUFFLE:
        {
            // TODO: Touch to swap
        
            if (UpdateTimerLoop(mDelayTimer, dt, kStateTimeDelayLong))
            {
                mShuffleUiIndex = (mShuffleUiIndex + 1) % 3;
            }
            break;
        }
        case GAME_STATE_SHUFFLE_SHUFFLING:
        {
            if (mSwapAnimationSlideTimer <= 0.0f)
            {
                if (UpdateTimer(mDelayTimer, dt))
                {
                    ShufflePieces(kNumCubes);
                }
            }
            break;
        }
        case GAME_STATE_SHUFFLE_UNSHUFFLE_THE_FACES:
        {
            if (UpdateTimerLoop(mDelayTimer, dt, kStateTimeDelayLong))
            {
                mShuffleUiIndex = (mShuffleUiIndex + 1) % 2;
            }
            
            if (AnyTouchBegin())
            {
                StartGameState(GAME_STATE_SHUFFLE_PLAY);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_PLAY:
        {
            mScoreTimer += dt;
            
            for (unsigned int i = 0; i < arraysize(mFaceCompleteTimers); ++i)
            {
                if (mFaceCompleteTimers[i] > 0.0f)
                {
                    UpdateTimer(mFaceCompleteTimers[i], dt);
                }
            }
            
            if (mHintTimer > 0.0f && mSwapState == SWAP_STATE_NONE)
            {
                if (UpdateTimer(mHintTimer, dt))
                {
                    StartGameState(GAME_STATE_SHUFFLE_HINT);
                }
            }
            break;
        }
        case GAME_STATE_SHUFFLE_HINT:
        {
            for (unsigned int i = 0; i < arraysize(mFaceCompleteTimers); ++i)
            {
                if (mFaceCompleteTimers[i] > 0.0f)
                {
                    UpdateTimer(mFaceCompleteTimers[i], dt);
                }
            }
            
            if (UpdateTimer(mHintTimer, dt))
            {
                StopHint();
                StartGameState(GAME_STATE_SHUFFLE_PLAY);
            }
            
            if (AnyTouchBegin())
            {
                StopHint();
                StartGameState(GAME_STATE_SHUFFLE_PLAY);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_SOLVED:
        {
            if (UpdateTimer(mDelayTimer, dt))
            {
                StartGameState(GAME_STATE_SHUFFLE_CONGRATULATIONS);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_CONGRATULATIONS:
        {
            if (UpdateTimer(mDelayTimer, dt))
            {
                StartGameState(GAME_STATE_SHUFFLE_SCORE);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_SCORE:
        {
            if (arraysize(mTouching) > 2 && mTouching[2] == TOUCH_STATE_BEGIN)
            {
                StartGameState(GAME_STATE_MAIN_MENU);
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
                if (++mStoryCutsceneIndex == GetPuzzle(mStoryPuzzleIndex).GetNumCutsceneTextStart())
                {
                    StartGameState(GAME_STATE_STORY_DISPLAY_START_STATE);
                }
                else
                {
                    mDelayTimer += kCutsceneTextDelay;
                }
            }
            break;
        }
        case GAME_STATE_STORY_DISPLAY_START_STATE:
        {
            if (UpdateTimer(mDelayTimer, dt))
            {
                if (GetPuzzle(mStoryPuzzleIndex).GetNumShuffles() > 0)
                {
                    StartGameState(GAME_STATE_STORY_SCRAMBLING);
                }
                else
                {
                    StartGameState(GAME_STATE_STORY_CLUE);
                }
            }
            break;
        }
        case GAME_STATE_STORY_SCRAMBLING:
        {
            if (mSwapAnimationSlideTimer <= 0.0f)
            {
                if (UpdateTimer(mDelayTimer, dt))
                {
                    ShufflePieces(GetPuzzle(mStoryPuzzleIndex).GetNumBuddies());
                }
            }
            break;
        }
        case GAME_STATE_STORY_CLUE:
        {
            if (AnyTouchBegin())
            {
                StartGameState(GAME_STATE_STORY_PLAY);
            }
            break;
        }
        case GAME_STATE_STORY_PLAY:
        {
            mScoreTimer += dt;
            
            for (unsigned int i = 0; i < arraysize(mFaceCompleteTimers); ++i)
            {
                if (mFaceCompleteTimers[i] > 0.0f)
                {
                    UpdateTimer(mFaceCompleteTimers[i], dt);
                }
            }
            
            if (mHintTimer > 0.0f && mSwapState == SWAP_STATE_NONE)
            {
                if (UpdateTimer(mHintTimer, dt))
                {
                    StartGameState(GAME_STATE_STORY_HINT_MOVE);
                }
            }
            
            // TODO: Allow all cubes as hints?
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    if (mTouching[i] == TOUCH_STATE_BEGIN)
                    {
                        mHintCubeTouched = i;
                        StartGameState(GAME_STATE_STORY_HINT_CLUE);
                        break;
                    }
                }
            }
            break;
        }
        case GAME_STATE_STORY_HINT_CLUE:
        {
            mScoreTimer += dt;
            
            for (unsigned int i = 0; i < arraysize(mFaceCompleteTimers); ++i)
            {
                if (mFaceCompleteTimers[i] > 0.0f)
                {
                    UpdateTimer(mFaceCompleteTimers[i], dt);
                }
            }
            
            if (UpdateTimer(mDelayTimer, dt))
            {
                mHintCubeTouched = CUBE_ID_UNDEFINED;
                StartGameState(GAME_STATE_STORY_PLAY);
            }
            
            if (AnyTouchBegin())
            {
                mHintCubeTouched = CUBE_ID_UNDEFINED;
                StartGameState(GAME_STATE_STORY_PLAY);
            }
            break;
        }
        case GAME_STATE_STORY_HINT_MOVE:
        {
            mScoreTimer += dt;
            
            for (unsigned int i = 0; i < arraysize(mFaceCompleteTimers); ++i)
            {
                if (mFaceCompleteTimers[i] > 0.0f)
                {
                    UpdateTimer(mFaceCompleteTimers[i], dt);
                }
            }
            
            if (UpdateTimer(mHintTimer, dt))
            {
                StopHint();
                StartGameState(GAME_STATE_STORY_PLAY);
            }
            
            if (AnyTouchBegin())
            {
                StopHint();
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
                if (++mStoryCutsceneIndex == GetPuzzle(mStoryPuzzleIndex).GetNumCutsceneTextEnd())
                {
                    StartGameState(GAME_STATE_STORY_CHAPTER_END);
                }
                else
                {
                    mDelayTimer += kCutsceneTextDelay;
                }
            }
            break;
        }
        case GAME_STATE_STORY_CHAPTER_END:
        {
            if (arraysize(mTouching) > 0 && mTouching[0] == TOUCH_STATE_BEGIN)
            {
                mStoryPuzzleIndex = (mStoryPuzzleIndex + 1) % GetNumPuzzles();
                StartGameState(GAME_STATE_STORY_CHAPTER_START);
            }
            else if (arraysize(mTouching) > 1 && mTouching[1] == TOUCH_STATE_BEGIN)
            {
                StartGameState(GAME_STATE_STORY_CHAPTER_START);
            }
            else if (arraysize(mTouching) > 2 && mTouching[2] == TOUCH_STATE_BEGIN)
            {
                StartGameState(GAME_STATE_MAIN_MENU);
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
        case GAME_STATE_MAIN_MENU:
        {
            if (cubeWrapper.GetId() == 0)
            {
                cubeWrapper.DrawBackground(MainMenuChoices);
            }
            else if (cubeWrapper.GetId() == 1)
            {
                cubeWrapper.DrawBackground(MainMenuSelector);
            }
            else
            {
                cubeWrapper.DrawBackground(UiBackground);
            }
        
            break;
        }
        case GAME_STATE_FREE_PLAY:
        {
            cubeWrapper.DrawBuddy();
            break;
        }
        case GAME_STATE_SHUFFLE_TITLE:
        {
            cubeWrapper.DrawBackground(ShuffleTitleScreen);
            break;
        }
        case GAME_STATE_SHUFFLE_CHARACTER_SPLASH:
        {
            cubeWrapper.DrawBuddy();
            break;
        }
        case GAME_STATE_SHUFFLE_SHAKE_TO_SHUFFLE:
        {
            cubeWrapper.DrawBackground(GetBuddyFullAsset(cubeWrapper.GetBuddyId()));
            
            if (mShuffleUiIndex == 0 || mShuffleUiIndex == 1)
            {
                unsigned int bannerIndex = (mShuffleUiIndex + cubeWrapper.GetId()) % 2;
                cubeWrapper.DrawUiAsset(Vec2(0, 0), bannerIndex ? ShuffleShakeToShuffle : ShuffleTouchToSwap);
            }
            else if (mShuffleUiIndex == 2)
            {
                // Don't display a banner in this case
            }
            break;
        }
        case GAME_STATE_SHUFFLE_SHUFFLING:
        {
            cubeWrapper.DrawBuddy();
            break;
        }
        case GAME_STATE_SHUFFLE_UNSHUFFLE_THE_FACES:
        {
            if (cubeWrapper.GetId() == 0 && mShuffleUiIndex == 1)
            {
                cubeWrapper.DrawBackground(ShuffleNeighbor);
            }
            else
            {
                cubeWrapper.DrawBuddy();
            }
            break;
        }
        case GAME_STATE_SHUFFLE_PLAY:
        case GAME_STATE_SHUFFLE_HINT:
        {
            if (mFaceCompleteTimers[cubeWrapper.GetId()] > 0.0f)
            {
                cubeWrapper.DrawBackground(UiBackground);
                cubeWrapper.DrawUiAsset(
                    Vec2(0, 0),
                    cubeWrapper.GetId() == 0 ? FaceCompleteBlue : FaceCompleteOrange);
            }
            else
            {
                cubeWrapper.DrawBuddy();
            }
            break;
        }
        case GAME_STATE_SHUFFLE_SOLVED:
        {
            if (mFaceCompleteTimers[cubeWrapper.GetId()] > 0.0f)
            {
                cubeWrapper.DrawBackground(UiBackground);
                cubeWrapper.DrawUiAsset(
                    Vec2(0, 0),
                    cubeWrapper.GetId() == 0 ? FaceCompleteBlue : FaceCompleteOrange);
            }
            else
            {
                cubeWrapper.DrawBuddy();
            }
            break;
        }
        case GAME_STATE_SHUFFLE_CONGRATULATIONS:
        {
            cubeWrapper.DrawBackground(ShuffleCongratulations);
            // TODO: Bouncing sprites
            break;
        }
        case GAME_STATE_SHUFFLE_SCORE:
        {
            if (cubeWrapper.GetId() == 0 || cubeWrapper.GetId() > 2)
            {
                cubeWrapper.DrawBackground(ShufflePanelBestTimes);
                
                // TODO: Best Times
                // TODO: Ribbon
                //int minutes = int(mScoreTimer) / 60;
                //int seconds = int(mScoreTimer - (minutes * 60.0f));
                //DrawScoreBanner(cubeWrapper, minutes, seconds);
            }
            if (cubeWrapper.GetId() == 1)
            {
                cubeWrapper.DrawBackground(ShuffleShakeToPlayAgain);
            }
            else if (cubeWrapper.GetId() == 2)
            {
                cubeWrapper.DrawBackground(ShuffleTouchToExit);
            }
            break;
        }
        case GAME_STATE_STORY_CHAPTER_START:
        {
            DrawChapterTitle(cubeWrapper, mStoryPuzzleIndex);
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_START:
        {
            if (cubeWrapper.GetId() == 0)
            {
                cubeWrapper.DrawCutscene(GetPuzzle(mStoryPuzzleIndex).GetCutsceneTextStart(mStoryCutsceneIndex));
            }
            else
            {
                DrawChapterTitle(cubeWrapper, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_DISPLAY_START_STATE:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mStoryPuzzleIndex).GetNumBuddies())
            {
                cubeWrapper.DrawBuddy();
            }
            else
            {
                DrawChapterTitle(cubeWrapper, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_SCRAMBLING:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mStoryPuzzleIndex).GetNumBuddies())
            {
                cubeWrapper.DrawBuddy();
            }
            else
            {
                DrawChapterTitle(cubeWrapper, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_CLUE:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mStoryPuzzleIndex).GetNumBuddies())
            {
                cubeWrapper.DrawBackground(UiBackground);
                cubeWrapper.DrawUiAsset(Vec2(0, 3), ChapterOverlayNeighbor);
                cubeWrapper.DrawUiText(Vec2(2, 4), FontOrange, GetPuzzle(mStoryPuzzleIndex).GetClue());
            }
            else
            {
                DrawChapterTitle(cubeWrapper, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_PLAY:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mStoryPuzzleIndex).GetNumBuddies())
            {
                if (mFaceCompleteTimers[cubeWrapper.GetId()] > 0.0f)
                {
                    cubeWrapper.DrawBackground(UiBackground);
                    cubeWrapper.DrawUiAsset(
                        Vec2(0, 0),
                        cubeWrapper.GetId() == 0 ? FaceCompleteBlue : FaceCompleteOrange);
                }
                else
                {
                    cubeWrapper.DrawBuddy();
                }
            }
            else
            {
                DrawChapterTitle(cubeWrapper, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_HINT_CLUE:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mStoryPuzzleIndex).GetNumBuddies())
            {
                if (mFaceCompleteTimers[cubeWrapper.GetId()] > 0.0f)
                {
                    cubeWrapper.DrawBackground(UiBackground);
                    cubeWrapper.DrawUiAsset(
                        Vec2(0, 0),
                        cubeWrapper.GetId() == 0 ? FaceCompleteBlue : FaceCompleteOrange);
                }
                
                ASSERT(mHintCubeTouched != CUBE_ID_UNDEFINED);
                if (cubeWrapper.GetId() == mHintCubeTouched)
                {
                    cubeWrapper.DrawBackground(UiBackground);
                    cubeWrapper.DrawUiAsset(Vec2(0, 3), ChapterOverlay);
                    cubeWrapper.DrawUiText(Vec2(2, 4), FontOrange, GetPuzzle(mStoryPuzzleIndex).GetClue());
                }
                
                if (mFaceCompleteTimers[cubeWrapper.GetId()] == 0.0f &&
                    cubeWrapper.GetId() != mHintCubeTouched)
                {
                    cubeWrapper.DrawBuddy();
                }
            }
            else
            {
                DrawChapterTitle(cubeWrapper, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_HINT_MOVE:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mStoryPuzzleIndex).GetNumBuddies())
            {
                if (mFaceCompleteTimers[cubeWrapper.GetId()] > 0.0f)
                {
                    cubeWrapper.DrawBackground(UiBackground);
                    cubeWrapper.DrawUiAsset(
                        Vec2(0, 0),
                        cubeWrapper.GetId() == 0 ? FaceCompleteBlue : FaceCompleteOrange);
                }
                else
                {
                    cubeWrapper.DrawBuddy();
                }
            }
            else
            {
                DrawChapterTitle(cubeWrapper, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_SOLVED:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mStoryPuzzleIndex).GetNumBuddies())
            {
                if (mFaceCompleteTimers[cubeWrapper.GetId()] > 0.0f)
                {
                    cubeWrapper.DrawBackground(UiBackground);
                    cubeWrapper.DrawUiAsset(
                        Vec2(0, 0),
                        cubeWrapper.GetId() == 0 ? FaceCompleteBlue : FaceCompleteOrange);
                }
                else
                {
                    cubeWrapper.DrawBuddy();
                }
            }
            else
            {
                DrawChapterTitle(cubeWrapper, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_END:
        {
            if (cubeWrapper.GetId() == 0)
            {
                cubeWrapper.DrawCutscene(GetPuzzle(mStoryPuzzleIndex).GetCutsceneTextEnd(mStoryCutsceneIndex));
            }
            else
            {
                DrawChapterSummary(cubeWrapper, mStoryPuzzleIndex, mScoreTimer, mScoreMoves);
            }
            break;
        }
        case GAME_STATE_STORY_CHAPTER_END:
        {
            if (cubeWrapper.GetId() == 0)
            {
                DrawChapterNext(cubeWrapper, mStoryPuzzleIndex);
            }
            else if (cubeWrapper.GetId() == 1)
            {
                DrawChapterRetry(cubeWrapper, mStoryPuzzleIndex);
            }
            else if (cubeWrapper.GetId() == 2)
            {
                cubeWrapper.DrawBackground(ChapterExitToMenu);
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

void App::ShufflePieces(unsigned int numCubes)
{   
    ++mShuffleMoveCounter;
    
    unsigned int numPieces = numCubes * NUM_SIDES;
    ASSERT(numPieces <= arraysize(mShufflePiecesMoved));
    
    unsigned int piece0 = GetRandomNonMovedPiece(mShufflePiecesMoved, numPieces);
    unsigned int piece1 = GetRandomOtherPiece(mShufflePiecesMoved, numPieces, piece0);
    
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
                            
                            if (mCubeWrappers[iCube0].GetPieceSolution(iSide0).GetMustSolve() &&
                                piece0.GetAttribute() != Piece::ATTR_FIXED &&
                                mCubeWrappers[iCube1].GetPieceSolution(iSide1).GetMustSolve() &&
                                piece1.GetAttribute() != Piece::ATTR_FIXED)
                            {
                                const Piece &pieceSolution0 =
                                    mCubeWrappers[iCube0].GetPieceSolution(iSide0);
                                const Piece &pieceSolution1 =
                                    mCubeWrappers[iCube1].GetPieceSolution(iSide1);
                                
                                if (!piece0.Compare(pieceSolution0) &&
                                    !piece1.Compare(pieceSolution1))
                                {
                                    if (piece0.Compare(pieceSolution1) &&
                                        piece1.Compare(pieceSolution0))
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
                
                if (piece0.GetAttribute() != Piece::ATTR_FIXED && !piece0.Compare(pieceSolution0))
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
                                    
                                    if (piece0.Compare(pieceSolution1))
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
                
                if (piece0.GetAttribute() != Piece::ATTR_FIXED && !piece0.Compare(pieceSolution0))
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

void App::StartHint()
{
    ChooseHint();
    
    ASSERT(mHintPiece0 != -1);
    ASSERT(mHintPiece1 != -1);
    
    mCubeWrappers[mHintPiece0 / NUM_SIDES].StartPieceBlinking(mHintPiece0 % NUM_SIDES);
    mCubeWrappers[mHintPiece1 / NUM_SIDES].StartPieceBlinking(mHintPiece1 % NUM_SIDES);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::StopHint()
{
    ASSERT(mHintPiece0 != -1);
    ASSERT(mHintPiece1 != -1);
    
    mCubeWrappers[mHintPiece0 / NUM_SIDES].StopPieceBlinking();
    mCubeWrappers[mHintPiece1 / NUM_SIDES].StopPieceBlinking();
    
    mHintPiece0 = -1;
    mHintPiece1 = -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TODO: This swap logic is geting too complex...
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::UpdateSwap(float dt)
{
    if (mSwapState == SWAP_STATE_OUT)
    {
        bool done = UpdateTimer(mSwapAnimationSlideTimer, dt);
        
        float slide_tick = kSwapAnimationSlide / float(kSwapAnimationCount);
        int swap_anim_counter = mSwapAnimationSlideTimer / slide_tick;
        
        if ((mSwapPiece0 % NUM_SIDES) == SIDE_TOP || (mSwapPiece0 % NUM_SIDES) == SIDE_BOTTOM)
        {
            mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPieceOffset(
                mSwapPiece0 % NUM_SIDES,
                Vec2(0, -kSwapAnimationCount + swap_anim_counter));
        }
         else if ((mSwapPiece0 % NUM_SIDES) == SIDE_LEFT || (mSwapPiece0 % NUM_SIDES) == SIDE_RIGHT)
        {
            mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPieceOffset(
                mSwapPiece0 % NUM_SIDES,
                Vec2(-kSwapAnimationCount + swap_anim_counter, 0));
        }
        
        if ((mSwapPiece1 % NUM_SIDES) == SIDE_TOP || (mSwapPiece1 % NUM_SIDES) == SIDE_BOTTOM)
        {        
            mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPieceOffset(
                mSwapPiece1 % NUM_SIDES,
                Vec2(0, -kSwapAnimationCount + swap_anim_counter));
        }
        else if ((mSwapPiece1 % NUM_SIDES) == SIDE_LEFT || (mSwapPiece1 % NUM_SIDES) == SIDE_RIGHT)
        {
            mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPieceOffset(
                mSwapPiece1 % NUM_SIDES,
                Vec2(-kSwapAnimationCount + swap_anim_counter, 0));
        }
        
        if (done)
        {
            OnSwapExchange();
        }
    }
    else if (mSwapState == SWAP_STATE_IN)
    {
        if (mSwapAnimationSlideTimer > 0.0f)
        {
            UpdateTimer(mSwapAnimationSlideTimer, dt);
            
            float slide_tick = kSwapAnimationSlide / float(kSwapAnimationCount);
            int swap_anim_counter = mSwapAnimationSlideTimer / slide_tick;
            
            if ((mSwapPiece0 % NUM_SIDES) == SIDE_TOP || (mSwapPiece0 % NUM_SIDES) == SIDE_BOTTOM)
            {
                mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPieceOffset(
                    mSwapPiece0 % NUM_SIDES,
                    Vec2(0, -swap_anim_counter));
            }
            else if ((mSwapPiece0 % NUM_SIDES) == SIDE_LEFT || (mSwapPiece0 % NUM_SIDES) == SIDE_RIGHT)
            {
                mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPieceOffset(
                    mSwapPiece0 % NUM_SIDES,
                    Vec2(-swap_anim_counter, 0));
            }
            
            if ((mSwapPiece1 % NUM_SIDES) == SIDE_TOP || (mSwapPiece1 % NUM_SIDES) == SIDE_BOTTOM)
            {
                mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPieceOffset(
                    mSwapPiece1 % NUM_SIDES,
                    Vec2(0, -swap_anim_counter));
            }
            else if ((mSwapPiece1 % NUM_SIDES) == SIDE_LEFT || (mSwapPiece1 % NUM_SIDES) == SIDE_RIGHT)
            {
                mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPieceOffset(
                    mSwapPiece1 % NUM_SIDES,
                    Vec2(-swap_anim_counter, 0));
            }
        }
        
        if (mSwapAnimationRotateTimer > 0.0f)
        {
            if (UpdateTimer(mSwapAnimationRotateTimer, dt))
            {            
                Piece piece0 = mCubeWrappers[mSwapPiece0 / NUM_SIDES].GetPiece(mSwapPiece0 % NUM_SIDES);
                if (!IsAtRotationTarget(piece0, mSwapPiece0 % NUM_SIDES))
                {
                    piece0.SetRotation(piece0.GetRotation() - 1);
                    if (piece0.GetRotation() < 0)
                    {
                        piece0.SetRotation(piece0.GetRotation() + NUM_SIDES);
                    }
                }
                mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPiece(mSwapPiece0 % NUM_SIDES, piece0);
                
                Piece piece1 = mCubeWrappers[mSwapPiece1 / NUM_SIDES].GetPiece(mSwapPiece1 % NUM_SIDES);
                if (!IsAtRotationTarget(piece1, mSwapPiece1 % NUM_SIDES))
                {
                    piece1.SetRotation(piece1.GetRotation() - 1);
                    if (piece1.GetRotation() < 0)
                    {
                        piece1.SetRotation(piece1.GetRotation() + NUM_SIDES);
                    }
                }
                mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPiece(mSwapPiece1 % NUM_SIDES, piece1);
                
                if (!IsAtRotationTarget(piece0, mSwapPiece0 % NUM_SIDES) ||
                    !IsAtRotationTarget(piece1, mSwapPiece1 % NUM_SIDES))
                {
                    mSwapAnimationRotateTimer += kSwapAnimationSlide / NUM_SIDES;
                }
            }
        }
        
        // Are we done here?
        const Piece &piece0 = mCubeWrappers[mSwapPiece0 / NUM_SIDES].GetPiece(mSwapPiece0 % NUM_SIDES);
        const Piece &piece1 = mCubeWrappers[mSwapPiece1 / NUM_SIDES].GetPiece(mSwapPiece1 % NUM_SIDES);
        
        if (mSwapAnimationSlideTimer <= 0.0f &&
            IsAtRotationTarget(piece0, mSwapPiece0 % NUM_SIDES) &&
            IsAtRotationTarget(piece1, mSwapPiece1 % NUM_SIDES))
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
    mSwapAnimationSlideTimer = kSwapAnimationSlide;
    
    mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPieceOffset(mSwapPiece0 % NUM_SIDES, Vec2(0, 0));
    mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPieceOffset(mSwapPiece1 % NUM_SIDES, Vec2(0, 0));
    
    mFaceCompleteTimers[mSwapPiece0 / NUM_SIDES] = 0.0f;
    mFaceCompleteTimers[mSwapPiece1 / NUM_SIDES] = 0.0f;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////              

void App::OnSwapExchange()
{
    mSwapState = SWAP_STATE_IN;
    mSwapAnimationSlideTimer = kSwapAnimationSlide;
    mSwapAnimationRotateTimer = kSwapAnimationSlide / NUM_SIDES;
    
    int side0 = mSwapPiece0 % NUM_SIDES;
    int side1 = mSwapPiece1 % NUM_SIDES;
    
    Piece piece0 = mCubeWrappers[mSwapPiece0 / NUM_SIDES].GetPiece(side0);
    Piece piece1 = mCubeWrappers[mSwapPiece1 / NUM_SIDES].GetPiece(side1);
    
    Piece piece0new = piece1;
    Piece piece1new = piece0;
    
    if (side0 == side1)
    {
        piece0new.SetRotation(piece0new.GetRotation() + 2);
        piece1new.SetRotation(piece1new.GetRotation() + 2);
    }
    else if (side0 % 2 != side1 % 2)
    {
        if ((side0 + 1) % NUM_SIDES == side1)
        {
            piece0new.SetRotation(piece0new.GetRotation() + 1);
            piece1new.SetRotation(piece1new.GetRotation() + 3);
        }
        else
        {
            piece0new.SetRotation(piece0new.GetRotation() + 3);
            piece1new.SetRotation(piece1new.GetRotation() + 1);
        }
    }
    
    piece0new.SetRotation(ClampMod(piece0new.GetRotation(), NUM_SIDES));
    piece1new.SetRotation(ClampMod(piece1new.GetRotation(), NUM_SIDES));
    
    mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPiece(side0, piece0new);
    mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPiece(side1, piece1new);
    
    // Mark both as moved...
    mShufflePiecesMoved[mSwapPiece0] = true;
    mShufflePiecesMoved[mSwapPiece1] = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::OnSwapFinish()
{
    mSwapState = SWAP_STATE_NONE;
    mSwapAnimationSlideTimer = 0.0f;
    
    mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPieceOffset(mSwapPiece0 % NUM_SIDES, Vec2(0, 0));
    mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPieceOffset(mSwapPiece1 % NUM_SIDES, Vec2(0, 0));
    
    if (mGameState == GAME_STATE_FREE_PLAY)
    {
        if (mCubeWrappers[mSwapPiece0 / NUM_SIDES].IsSolved() ||
            mCubeWrappers[mSwapPiece1 / NUM_SIDES].IsSolved())
        {
            PlaySound();
        }
    }
    else if (mGameState == GAME_STATE_SHUFFLE_SHUFFLING)
    {
        bool done = GetNumMovedPieces(
            mShufflePiecesMoved, arraysize(mShufflePiecesMoved)) == arraysize(mShufflePiecesMoved);
        
        if (kShuffleMaxMoves > 0)
        {
            done = done || mShuffleMoveCounter == (unsigned int)kShuffleMaxMoves;
        }
        
        if (done)
        {
            StartGameState(GAME_STATE_SHUFFLE_UNSHUFFLE_THE_FACES);
        }
        else
        {
            mDelayTimer += kShuffleScrambleTimerDelay;
        }
    }
    else if (mGameState == GAME_STATE_SHUFFLE_PLAY)
    {
        bool swap0Solved =
            mCubeWrappers[mSwapPiece0 / NUM_SIDES].IsSolved() &&
            mCubeWrappers[mSwapPiece0 / NUM_SIDES].GetPiece(mSwapPiece0 % NUM_SIDES).Compare(
                mCubeWrappers[mSwapPiece0 / NUM_SIDES].GetPieceSolution(mSwapPiece0 % NUM_SIDES)) &&
            mCubeWrappers[mSwapPiece0 / NUM_SIDES].GetPieceSolution(mSwapPiece0 % NUM_SIDES).GetMustSolve();
        
        bool swap1Solved =
            mCubeWrappers[mSwapPiece1 / NUM_SIDES].IsSolved() &&
            mCubeWrappers[mSwapPiece1 / NUM_SIDES].GetPiece(mSwapPiece1 % NUM_SIDES).Compare(
                mCubeWrappers[mSwapPiece1 / NUM_SIDES].GetPieceSolution(mSwapPiece1 % NUM_SIDES)) &&     
            mCubeWrappers[mSwapPiece1 / NUM_SIDES].GetPieceSolution(mSwapPiece1 % NUM_SIDES).GetMustSolve();
        
        if (swap0Solved || swap1Solved)
        {
            PlaySound();
        }
        
        if (swap0Solved)
        {
            mFaceCompleteTimers[mSwapPiece0 / NUM_SIDES] = kShuffleFaceCompleteTimerDuration;
        }
        
        if (swap1Solved)
        {
            mFaceCompleteTimers[mSwapPiece1 / NUM_SIDES] = kShuffleFaceCompleteTimerDuration;
        }
        
        if (AllSolved(*this))
        {
            StartGameState(GAME_STATE_SHUFFLE_SOLVED);
        }
    }
    else if (mGameState == GAME_STATE_STORY_SCRAMBLING)
    {
        bool done = GetNumMovedPieces(
            mShufflePiecesMoved, arraysize(mShufflePiecesMoved)) == arraysize(mShufflePiecesMoved);
        
        if (GetPuzzle(mStoryPuzzleIndex).GetNumShuffles() > 0)
        {
            done = done || mShuffleMoveCounter == GetPuzzle(mStoryPuzzleIndex).GetNumShuffles();
        }
        
        if (done)
        {
            StartGameState(GAME_STATE_STORY_CLUE);
        }
        else
        {
            mDelayTimer += kShuffleScrambleTimerDelay;
        }
    }
    else if (mGameState == GAME_STATE_STORY_PLAY)
    {
        bool swap0Solved =
            mCubeWrappers[mSwapPiece0 / NUM_SIDES].IsSolved() &&
            mCubeWrappers[mSwapPiece0 / NUM_SIDES].GetPiece(mSwapPiece0 % NUM_SIDES).Compare(
                mCubeWrappers[mSwapPiece0 / NUM_SIDES].GetPieceSolution(mSwapPiece0 % NUM_SIDES)) &&
            mCubeWrappers[mSwapPiece0 / NUM_SIDES].GetPieceSolution(mSwapPiece0 % NUM_SIDES).GetMustSolve();
        
        bool swap1Solved =
            mCubeWrappers[mSwapPiece1 / NUM_SIDES].IsSolved() &&
            mCubeWrappers[mSwapPiece1 / NUM_SIDES].GetPiece(mSwapPiece1 % NUM_SIDES).Compare(
                mCubeWrappers[mSwapPiece1 / NUM_SIDES].GetPieceSolution(mSwapPiece1 % NUM_SIDES)) &&     
            mCubeWrappers[mSwapPiece1 / NUM_SIDES].GetPieceSolution(mSwapPiece1 % NUM_SIDES).GetMustSolve();
        
        if (swap0Solved || swap1Solved)
        {
            PlaySound();
        }
        
        if (swap0Solved)
        {
            mFaceCompleteTimers[mSwapPiece0 / NUM_SIDES] = kShuffleFaceCompleteTimerDuration;
        }
        
        if (swap1Solved)
        {
            mFaceCompleteTimers[mSwapPiece1 / NUM_SIDES] = kShuffleFaceCompleteTimerDuration;
        }
        
        if (AllSolved(*this))
        {
            StartGameState(GAME_STATE_STORY_SOLVED);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool App::AnyTouchBegin() const
{
    for (unsigned int i = 0; i < arraysize(mTouching); ++i)
    {
        if (mTouching[i] == TOUCH_STATE_BEGIN)
        {
            return true;
        }
    }
    
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool App::AnyTouchEnd() const
{
    for (unsigned int i = 0; i < arraysize(mTouching); ++i)
    {
        if (mTouching[i] == TOUCH_STATE_END)
        {
            return true;
        }
    }
    
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}
