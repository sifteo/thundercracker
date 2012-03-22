/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "App.h"
#include <cstdio>
#include <limits>
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

const int kMaxTilesX = VidMode::LCD_width / VidMode::TILE;
const int kMaxTilesY = VidMode::LCD_width / VidMode::TILE;

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

void ScoreTimerToTime(float scoreTimer, int &minutes, int &seconds)
{
    minutes = int(scoreTimer) / 60;
    seconds = int(scoreTimer - (minutes * 60.0f));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawOption(CubeWrapper &cubeWrapper, const char *option, bool displayTime, float scoreTimer = 0.0f)
{
    cubeWrapper.DrawBackground(UiPanel);
    
    String<16> bufferTouchTo;
    bufferTouchTo << "Touch to";
    int xTouchTo = (kMaxTilesX / 2) - (bufferTouchTo.size() / 2);
    cubeWrapper.DrawUiText(Vec2(xTouchTo, 4), UiFontHeadingOrange, bufferTouchTo.c_str());
    
    String<16> bufferOption;
    bufferOption << option;
    int xOption = (kMaxTilesX / 2) - (bufferOption.size() / 2);
    cubeWrapper.DrawUiText(Vec2(xOption, 6), UiFontHeadingOrange, bufferOption.c_str());
    
    if (displayTime)
    {
        int minutes, seconds;
        ScoreTimerToTime(scoreTimer, minutes, seconds);
        
        String<16> bufferTime;
        bufferTime << "Time " << Fixed(minutes, 2, true) << ":" << Fixed(seconds, 2, true);
        int xTime = (kMaxTilesX / 2) - (bufferTime.size() / 2);
        cubeWrapper.DrawUiText(Vec2(xTime, 10), UiFontOrange, bufferTime.c_str());
        
        if (bufferTouchTo.size() % 2 != 0)
        {
            cubeWrapper.ScrollUi(Vec2(VidMode::TILE / 2, 0));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawShuffleScore(
    const App &app,
    CubeWrapper &cubeWrapper,
    float scoreTimer,
    unsigned int place,
    const Vec2 &scroll)
{
    // Background
    const AssetImage *backgrounds[] =
    {
        &ShuffleBestTimesHighScore1,
        &ShuffleBestTimesHighScore2,
        &ShuffleBestTimesHighScore3,
    };
    const AssetImage &background = place < arraysize(backgrounds) ? *backgrounds[place] : ShuffleBestTimes;
    
    int xOffset = kMaxTilesX + scroll.x;
    
    cubeWrapper.DrawBackgroundPartial(
        Vec2(xOffset, 0),
        Vec2(0, 0),
        Vec2(-scroll.x, kMaxTilesY),
        background);
    
    // Doing text scroll on BG1 is a lotta work, so just pop it on when we hit target.
    if (xOffset == 0)
    {
        // Best Times Text
        const char *labels[] =
        {
            "1st ",
            "2nd ",
            "3rd ",
        };
        
        for (unsigned int i = 0; i < app.GetNumBestTimes(); ++i)
        {
            int minutes, seconds;
            ScoreTimerToTime(app.GetBestTime(i), minutes, seconds);
            
            String<16> buffer;
            buffer << labels[i] << Fixed(minutes, 2, true) << ":" << Fixed(seconds, 2, true);
            
            cubeWrapper.DrawUiText(
                Vec2(4, 4 + (i * 2)),
                place == i ? UiFontWhite : UiFontOrange,
                buffer.c_str());
        }
        
        // Optional "Your Score" for non-high scores
        if (place >= app.GetNumBestTimes())
        {
            int minutes, seconds;
            ScoreTimerToTime(scoreTimer, minutes, seconds);
            
            String<16> buffer;
            buffer << "Time " << Fixed(minutes, 2, true) << ":" << Fixed(seconds, 2, true);
        
            cubeWrapper.DrawUiText(Vec2(3 + (kMaxTilesX + scroll.x), 11), UiFontWhite, buffer.c_str());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawStoryChapterTitle(CubeWrapper &cubeWrapper, unsigned int puzzleIndex)
{
    cubeWrapper.DrawBackground(StoryChapterTitle);
    
    String<16> bufferChapter;
    bufferChapter << "Chapter " << (puzzleIndex + 1);
    int xChapter = (kMaxTilesX / 2) - (bufferChapter.size() / 2);
    cubeWrapper.DrawUiText(Vec2(xChapter, 6), UiFontHeadingOrange, bufferChapter.c_str());
    
    String<64> bufferTitle;
    bufferTitle << "\"" << GetPuzzle(puzzleIndex).GetTitle() << "\"";
    int xTitle = (kMaxTilesX / 2) - (bufferTitle.size() / 2);
    cubeWrapper.DrawUiText(Vec2(xTitle, 8), UiFontOrange, bufferTitle.c_str());
    
    if (bufferChapter.size() % 2 != 0)
    {
        cubeWrapper.ScrollUi(Vec2(VidMode::TILE / 2, 0));
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawStoryClue(
    CubeWrapper &cubeWrapper,
    unsigned int puzzleIndex,
    const AssetImage &background,
    const char *text)
{
    cubeWrapper.DrawBackground(background);
    
    String<32> bufferText;
    bufferText << text;
    
    int xText = (kMaxTilesX / 2) - (bufferText.size() / 2);
    cubeWrapper.DrawUiText(Vec2(xText, 5), UiFontOrange, bufferText.c_str());
    
    if (bufferText.size() % 2 != 0)
    {
        cubeWrapper.ScrollUi(Vec2(VidMode::TILE / 2, 0));
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawStoryFaceComplete(CubeWrapper &cubeWrapper)
{
    cubeWrapper.DrawBackground(StoryFaceComplete);
    
    String<32> buffer;
    buffer << "Face Solved!";
    
    int x = (kMaxTilesX / 2) - (buffer.size() / 2);
    cubeWrapper.DrawUiText(Vec2(x, 7), UiFontWhite, buffer.c_str());
    
    if (buffer.size() % 2 != 0)
    {
        cubeWrapper.ScrollUi(Vec2(VidMode::TILE / 2, 0));
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawStoryChapterSummary(CubeWrapper &cubeWrapper, unsigned int puzzleIndex)
{
    cubeWrapper.DrawBackground(StoryProgress);
    
    String<32> buffer0;
    buffer0 << (puzzleIndex + 1) << "/" << GetNumPuzzles() << " Puzzles";
    int x0 = (kMaxTilesX / 2) - (buffer0.size() / 2);
    cubeWrapper.DrawUiText(Vec2(x0, 6), UiFontOrange, buffer0.c_str());
    
    String<32> buffer1;
    buffer1 << "Solved!";
    int x1 = (kMaxTilesX / 2) - (buffer1.size() / 2);
    cubeWrapper.DrawUiText(Vec2(x1, 8), UiFontOrange, buffer1.c_str());
    
    if (buffer0.size() % 2 != 0)
    {
        cubeWrapper.ScrollUi(Vec2(VidMode::TILE / 2, 0));
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawStoryChapterNext(CubeWrapper &cubeWrapper, unsigned int puzzleIndex)
{
    cubeWrapper.DrawBackground(StoryChapterNext);
    
    unsigned int nextPuzzleIndex = ++puzzleIndex % GetNumPuzzles();
    
    String<16> buffer;
    buffer << "Chapter " << (nextPuzzleIndex + 1);
    int x = (kMaxTilesX / 2) - (buffer.size() / 2);
    
    cubeWrapper.DrawUiText(Vec2(x, 8), UiFontOrange, buffer.c_str());
    
    if (buffer.size() % 2 != 0)
    {
        cubeWrapper.ScrollUi(Vec2(VidMode::TILE / 2, 0));
    }
}
                    
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawStoryChapterRetry(CubeWrapper &cubeWrapper, unsigned int puzzleIndex)
{
    cubeWrapper.DrawBackground(StoryChapterRetry);
    
    String<16> buffer;
    buffer << "Chapter " << (puzzleIndex + 1);
    int x = (kMaxTilesX / 2) - (buffer.size() / 2);
    
    cubeWrapper.DrawUiText(Vec2(x, 8), UiFontOrange, buffer.c_str());
    
    if (buffer.size() % 2 != 0)
    {
        cubeWrapper.ScrollUi(Vec2(VidMode::TILE / 2, 0));
    }
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

unsigned int GetRandomOtherBuddyId(App &app, unsigned int buddyId)
{
    Random random;
    int selection = random.randrange(kMaxBuddies - kNumCubes);
    
    for (int j = 0; j < selection; ++j)
    {
        buddyId = (buddyId + 1) % kMaxBuddies;
        while (IsBuddyUsed(app, buddyId))
        {
            buddyId = (buddyId + 1) % kMaxBuddies;
        }
    }
    
    return buddyId;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void TiltNudgePieces(App& app, Cube::ID cubeId)
{
#ifdef BUDDY_PIECES_USE_SPRITES
    Vec2 accelState = app.GetCubeWrapper(cubeId).GetAccelState();
    float x = float(accelState.x + 61) / (123.0f * 0.5f) - 1.0f;
    float y = float(accelState.y + 61) / (123.0f * 0.5f) - 1.0f;
    float d = 8.0f;
    app.GetCubeWrapper(cubeId).SetPieceOffset(SIDE_TOP,    Vec2( x * d,  y * d));
    app.GetCubeWrapper(cubeId).SetPieceOffset(SIDE_LEFT,   Vec2( x * d,  y * d));
    app.GetCubeWrapper(cubeId).SetPieceOffset(SIDE_BOTTOM, Vec2(-x * d, -y * d));
    app.GetCubeWrapper(cubeId).SetPieceOffset(SIDE_RIGHT,  Vec2(-x * d,  y * d));
#else
    Cube::TiltState tiltState = app.GetCubeWrapper(cubeId).GetTiltState();
    
    app.GetCubeWrapper(cubeId).SetPieceOffset(
        SIDE_TOP,
        Vec2((tiltState.x - 1) * VidMode::TILE, (tiltState.y - 1) * VidMode::TILE));
    app.GetCubeWrapper(cubeId).SetPieceOffset(
        SIDE_LEFT,
        Vec2((tiltState.x - 1) * VidMode::TILE, (tiltState.y - 1) * VidMode::TILE));
    app.GetCubeWrapper(cubeId).SetPieceOffset(
        SIDE_BOTTOM,
        Vec2(-(tiltState.x - 1) * VidMode::TILE, -(tiltState.y - 1) * VidMode::TILE));
    app.GetCubeWrapper(cubeId).SetPieceOffset(
        SIDE_RIGHT,
        Vec2(-(tiltState.x - 1) * VidMode::TILE, (tiltState.y - 1) * VidMode::TILE));
#endif        
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

const char *kGameStateNames[NUM_GAME_STATES] =
{
    "GAME_STATE_NONE",
    "GAME_STATE_MAIN_MENU",
    "GAME_STATE_FREEPLAY_START",
    "GAME_STATE_FREEPLAY_PLAY",
    "GAME_STATE_FREEPLAY_OPTIONS",
    "GAME_STATE_SHUFFLE_START",
    "GAME_STATE_SHUFFLE_TITLE",
    "GAME_STATE_SHUFFLE_MEMORIZE_FACES",
    "GAME_STATE_SHUFFLE_CHARACTER_SPLASH",
    "GAME_STATE_SHUFFLE_SHAKE_TO_SHUFFLE",
    "GAME_STATE_SHUFFLE_SHUFFLING",
    "GAME_STATE_SHUFFLE_UNSHUFFLE_THE_FACES",
    "GAME_STATE_SHUFFLE_PLAY",
    "GAME_STATE_SHUFFLE_OPTIONS",
    "GAME_STATE_SHUFFLE_SOLVED",
    "GAME_STATE_SHUFFLE_CONGRATULATIONS",
    "GAME_STATE_SHUFFLE_END_GAME_NAV",
    "GAME_STATE_STORY_START",
    "GAME_STATE_STORY_CHAPTER_START",
    "GAME_STATE_STORY_CUTSCENE_START",
    "GAME_STATE_STORY_DISPLAY_START_STATE",
    "GAME_STATE_STORY_SCRAMBLING",
    "GAME_STATE_STORY_CLUE",
    "GAME_STATE_STORY_PLAY",
    "GAME_STATE_STORY_OPTIONS",
    "GAME_STATE_STORY_SOLVED",
    "GAME_STATE_STORY_CUTSCENE_END_1",
    "GAME_STATE_STORY_CUTSCENE_END_2",
    "GAME_STATE_STORY_UNLOCKED_1",
    "GAME_STATE_STORY_UNLOCKED_2",
    "GAME_STATE_STORY_UNLOCKED_3",
    "GAME_STATE_STORY_UNLOCKED_4",
    "GAME_STATE_STORY_CHAPTER_END",
};

const int kSwapAnimationCount = 64 - 8; // Note: piceces are offset by 8 pixels by design

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
    , mOptionsTimer(0.0f)
    , mOptionsTouchSync(false)
    , mUiIndex(0)
    , mUiIndexSync()
    , mTouching()
    , mScoreTimer(0.0f)
    , mScoreMoves(0)
    , mScorePlace(std::numeric_limits<unsigned int>::max())
    , mSaveDataStoryProgress(0)
    , mSaveDataBestTimes()
    , mSwapState(SWAP_STATE_NONE)
    , mSwapPiece0(0)
    , mSwapPiece1(0)
    , mSwapAnimationSlideTimer(0)
    , mSwapAnimationRotateTimer(0.0f)
    , mFaceCompleteTimers()
    , mBackgroundScroll(0, 0)
    , mHintTimer(0.0f)
    , mHintPiece0(-1)
    , mHintPiece1(-1)
    , mHintPieceSkip(-1)
    , mHintFlowIndex(0)
    , mClueOnTimer(0.0f)
    , mClueOffTimers()
    , mFreePlayShakeThrottleTimer(0.0f)
    , mShufflePiecesStart()
    , mShuffleMoveCounter(0)
    , mStoryPuzzleIndex(0)
    , mStoryCutsceneIndex(0)
{
    for (unsigned int i = 0; i < arraysize(mTouching); ++i)
    {
        mTouching[i] = TOUCH_STATE_NONE;
    }
    
    // Default high scores (will overwritten if save file is found)
    mSaveDataBestTimes[0] =  5.0f * 60.0f;
    mSaveDataBestTimes[1] = 10.0f * 60.0f;
    mSaveDataBestTimes[2] = 20.0f * 60.0f;
    
    for (unsigned int i = 0; i < arraysize(mFaceCompleteTimers); ++i)
    {
        mFaceCompleteTimers[i] = 0.0f;
    }
    
    for (unsigned int i = 0; i < arraysize(mClueOffTimers); ++i)
    {
        mClueOffTimers[i] = 0.0f;
    }
    
    for (unsigned int i = 0; i < arraysize(mUiIndexSync); ++i)
    {
        mUiIndexSync[i] = false;
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
    
    // Check all books are sequential
    if (GetNumPuzzles() > 1)
    {
        for (unsigned int i = 1; i < GetNumPuzzles(); ++i)
        {
            ASSERT(GetPuzzle(i).GetBook() >= GetPuzzle(i - 1).GetBook());
        }
    }
    
    LoadData();
    
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
                    StartGameState(GAME_STATE_FREEPLAY_START);
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
                    StartGameState(GAME_STATE_FREEPLAY_START);
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
    else
    {
        if (mGameState == GAME_STATE_SHUFFLE_UNSHUFFLE_THE_FACES)
        {
            StartGameState(GAME_STATE_SHUFFLE_PLAY);
        }
        
        // Hints
        if (IsHinting())
        {
            StopHint(true);
        }
        
        // Clues
        if (mGameState == GAME_STATE_SHUFFLE_PLAY)
        {
            mHintTimer = kHintTimerOnDuration; // In case neighbor happens between hinting cycles
            mHintFlowIndex = 0;
            mClueOnTimer = kClueTimerOnDuration;
        }
        else if (mGameState == GAME_STATE_STORY_PLAY)
        {
            mHintTimer = kHintTimerOnDuration; // In case neighbor happens between hinting cycles
            mHintFlowIndex = 0;
        }
        
        for (unsigned int i = 0; i < arraysize(mClueOffTimers); ++i)
        {
            mClueOffTimers[i] = 0.0f;
        }
        
        bool isSwapping = mSwapState != SWAP_STATE_NONE;
        
        bool isFixed =
            GetCubeWrapper(cubeId0).GetPiece(cubeSide0).GetAttribute() == Piece::ATTR_FIXED ||
            GetCubeWrapper(cubeId1).GetPiece(cubeSide1).GetAttribute() == Piece::ATTR_FIXED;
        
        bool isValidGameState =
            mGameState == GAME_STATE_FREEPLAY_PLAY ||
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
    switch (mGameState)
    {
        case GAME_STATE_FREEPLAY_PLAY:
        {
            TiltNudgePieces(*this, cubeId);
            break;
        }
        case GAME_STATE_SHUFFLE_UNSHUFFLE_THE_FACES:
        {
            StartGameState(GAME_STATE_SHUFFLE_PLAY);
            break;
        }
        case GAME_STATE_SHUFFLE_PLAY:
        {
            if (mSwapState == SWAP_STATE_NONE)
            {
                TiltNudgePieces(*this, cubeId);
            }
            
            if (IsHinting())
            {
                StopHint(false);
            }
            
            ASSERT(cubeId < arraysize(mClueOffTimers));
            if (mClueOffTimers[cubeId] > 0.0f)
            {
                mClueOffTimers[cubeId] = 0.0f;
            }
            mClueOnTimer = kClueTimerOnDuration;
            break;
        }
        case GAME_STATE_STORY_CLUE:
        {
            StartGameState(GAME_STATE_STORY_PLAY);
            break;
        }
        case GAME_STATE_STORY_PLAY:
        {
            if (mSwapState == SWAP_STATE_NONE)
            {
                TiltNudgePieces(*this, cubeId);
            }
            
            if (IsHinting())
            {
                StopHint(false);
            }
            
            ASSERT(cubeId < arraysize(mClueOffTimers));
            if (mClueOffTimers[cubeId] > 0.0f)
            {
                mClueOffTimers[cubeId] = 0.0f;
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

void App::OnShake(Cube::ID cubeId)
{
    switch (mGameState)
    {
        case GAME_STATE_FREEPLAY_PLAY:
        {
            if (mSwapState == SWAP_STATE_NONE && mFreePlayShakeThrottleTimer == 0.0f)
            {
                mFreePlayShakeThrottleTimer = kFreePlayShakeThrottleDuration;
            
                unsigned int newBuddyId = GetRandomOtherBuddyId(*this, mCubeWrappers[cubeId].GetBuddyId());
                mCubeWrappers[cubeId].SetBuddyId(newBuddyId);
                
                ResetCubesToPuzzle(GetPuzzleDefault(), false);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_SHAKE_TO_SHUFFLE:
        {
            StartGameState(GAME_STATE_SHUFFLE_SHUFFLING);
            break;
        }
        case GAME_STATE_SHUFFLE_PLAY:
        {   
            if (IsHinting())
            {
                StopHint(false);
            }
            
            ASSERT(cubeId < arraysize(mClueOffTimers));
            if (mClueOffTimers[cubeId] > 0.0f)
            {
                mClueOffTimers[cubeId] = 0.0f;
            }
            mClueOnTimer = kClueTimerOnDuration;
            break;
        }
        case GAME_STATE_STORY_CLUE:
        {
            StartGameState(GAME_STATE_STORY_PLAY);
            break;
        }
        case GAME_STATE_STORY_PLAY:
        {
            if (IsHinting())
            {
                StopHint(false);
            }
            
            ASSERT(cubeId < arraysize(mClueOffTimers));
            if (mClueOffTimers[cubeId] > 0.0f)
            {
                mClueOffTimers[cubeId] = 0.0f;
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

unsigned int App::GetNumBestTimes() const
{
    return arraysize(mSaveDataBestTimes);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

float App::GetBestTime(unsigned int place) const
{
    ASSERT(place < arraysize(mSaveDataBestTimes));
    return mSaveDataBestTimes[place];
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

void App::ResetCubesToShuffleStart()
{
    for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
    {
        if (mCubeWrappers[i].IsEnabled())
        {
            mCubeWrappers[i].Reset();
            
            for (unsigned int j = 0; j < NUM_SIDES; ++j)
            {
                mCubeWrappers[i].SetPiece(j, mShufflePiecesStart[i][j]);
                mCubeWrappers[i].SetPieceSolution(
                        j, GetPuzzleDefault().GetPieceEnd(mCubeWrappers[i].GetBuddyId(), j));
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
    mChannel.play(SoundGems);
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
        case GAME_STATE_FREEPLAY_START:
        {
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
            StartGameState(GAME_STATE_FREEPLAY_PLAY);
            break;
        }
        case GAME_STATE_FREEPLAY_PLAY:
        {
            mOptionsTimer = kOptionsTimerDuration;
            mFreePlayShakeThrottleTimer = 0.0f;
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
        case GAME_STATE_SHUFFLE_MEMORIZE_FACES:
        {
            mDelayTimer = kStateTimeDelayLong;
            mBackgroundScroll = Vec2(0, 0);
            break;
        }
        case GAME_STATE_SHUFFLE_CHARACTER_SPLASH:
        {
            mDelayTimer = kShuffleCharacterSplashDelay;
            break;
        }
        case GAME_STATE_SHUFFLE_SHAKE_TO_SHUFFLE:
        {
            mUiIndex = 0;
            mDelayTimer = kShuffleBannerSwapDelay;
            break;   
        }
        case GAME_STATE_SHUFFLE_SHUFFLING:
        {
            mDelayTimer = 0.0f;
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
            mScorePlace = std::numeric_limits<unsigned int>().max();
            mDelayTimer = kStateTimeDelayLong;
            mUiIndex = 0;
            for (unsigned int i = 0; i < arraysize(mUiIndexSync); ++i)
            {
                mUiIndexSync[i] = false;
            }
            
            // Copy in puzzle data so we can reset
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    for (unsigned int j = 0; j < NUM_SIDES; ++j)
                    {
                        ASSERT(i < arraysize(mShufflePiecesStart));
                        ASSERT(j < arraysize(mShufflePiecesStart[i]));
                        mShufflePiecesStart[i][j] = mCubeWrappers[i].GetPiece(j);
                    }
                }
            }
            break;
        }
        case GAME_STATE_SHUFFLE_PLAY:
        {
            mOptionsTimer = kOptionsTimerDuration;
            for (unsigned int i = 0; i < arraysize(mFaceCompleteTimers); ++i)
            {
                mFaceCompleteTimers[i] = 0.0f;
            }
            mHintTimer = kHintTimerOnDuration;
            mHintFlowIndex = 0;
            mClueOnTimer = kClueTimerOnDuration;
            break;
        }
        case GAME_STATE_SHUFFLE_SOLVED:
        {
            InsertScore();
            SaveData();
            
            mDelayTimer = kStateTimeDelayLong;
            for (unsigned int i = 0; i < arraysize(mFaceCompleteTimers); ++i)
            {
                mFaceCompleteTimers[i] = kShuffleFaceCompleteTimerDelay;
            }
            break;
        }
        case GAME_STATE_SHUFFLE_CONGRATULATIONS:
        {
            mDelayTimer = kStateTimeDelayLong;
            break;
        }
        case GAME_STATE_SHUFFLE_END_GAME_NAV:
        {
            mBackgroundScroll = Vec2(0, 0);
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
                mDelayTimer = kStoryCutsceneTextDelay;
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
        case GAME_STATE_STORY_CLUE:
        {
            mDelayTimer = kStateTimeDelayLong;
            break;
        }
        case GAME_STATE_STORY_PLAY:
        {
            mScoreTimer = 0.0f;
            mScoreMoves = 0;
            mOptionsTimer = kOptionsTimerDuration;
            for (unsigned int i = 0; i < arraysize(mFaceCompleteTimers); ++i)
            {
                mFaceCompleteTimers[i] = 0.0f;
            }
            for (unsigned int i = 0; i < arraysize(mClueOffTimers); ++i)
            {
                mClueOffTimers[i] = 0.0f;
            }
            mHintTimer = kHintTimerOnDuration;
            mHintFlowIndex = 0;
            break;
        }
        case GAME_STATE_STORY_SOLVED:
        {
            mDelayTimer = kStateTimeDelayLong;
            if ((mStoryPuzzleIndex + 1) > mSaveDataStoryProgress)
            {
                mSaveDataStoryProgress = mStoryPuzzleIndex + 1;
                SaveData();
            }
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_END_1:
        {
            mDelayTimer = kStateTimeDelayLong;
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_END_2:
        {
            if (GetPuzzle(mStoryPuzzleIndex).GetNumCutsceneTextEnd() == 1)
            {
                mDelayTimer = kStateTimeDelayLong;
            }
            else
            {
                mDelayTimer = kStoryCutsceneTextDelay;
            }
            mStoryCutsceneIndex = 0;
            break;
        }
        case GAME_STATE_STORY_UNLOCKED_1:
        {
            mBackgroundScroll = Vec2(0, 0);
            break;
        }
        case GAME_STATE_STORY_UNLOCKED_2:
        {
            mDelayTimer = kStateTimeDelayShort;
            mBackgroundScroll = Vec2(0, 0);
            break;
        }
        case GAME_STATE_STORY_UNLOCKED_3:
        {
            mDelayTimer = kStateTimeDelayShort;
            mBackgroundScroll = Vec2(0, 0);
            break;
        }
        case GAME_STATE_STORY_UNLOCKED_4:
        {
            mBackgroundScroll = Vec2(0, 0);
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
        case GAME_STATE_FREEPLAY_PLAY:
        {
            if (mSwapState == SWAP_STATE_NONE)
            {
                // Throttle shake input so players don't accidentally shake twice in a row
                if (mFreePlayShakeThrottleTimer > 0.0f)
                {
                    UpdateTimer(mFreePlayShakeThrottleTimer, dt);
                }
                
                // Check for holding, which enables the options menu.
                if (AnyTouchHold())
                {
                    if (UpdateTimer(mOptionsTimer, dt))
                    {
                        StartGameState(GAME_STATE_FREEPLAY_OPTIONS);
                    }
                }
                else
                {
                    mOptionsTimer = kOptionsTimerDuration;
                }
                
                for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
                {
                    if (mCubeWrappers[i].IsEnabled())
                    {
                        if (mTouching[i] == TOUCH_STATE_BEGIN || mTouching[i] == TOUCH_STATE_HOLD)
                        {
                            if (!mOptionsTouchSync)
                            {
                                mCubeWrappers[i].SetPieceOffset(SIDE_TOP,    Vec2(0, VidMode::TILE));
                                mCubeWrappers[i].SetPieceOffset(SIDE_LEFT,   Vec2(VidMode::TILE, 0));
                                mCubeWrappers[i].SetPieceOffset(SIDE_BOTTOM, Vec2(0, VidMode::TILE));
                                mCubeWrappers[i].SetPieceOffset(SIDE_RIGHT,  Vec2(VidMode::TILE, 0));
                            }
                        }
                        else if (mTouching[i] == TOUCH_STATE_END)
                        {
                            mOptionsTouchSync = false;
                            
                            mCubeWrappers[i].SetPieceOffset(SIDE_TOP,    Vec2(0, 0));
                            mCubeWrappers[i].SetPieceOffset(SIDE_LEFT,   Vec2(0, 0));
                            mCubeWrappers[i].SetPieceOffset(SIDE_BOTTOM, Vec2(0, 0));
                            mCubeWrappers[i].SetPieceOffset(SIDE_RIGHT,  Vec2(0, 0));
                        }
                        else
                        {
                            // If we're in sprite mode, we can do gradations of nudge, so
                            // poll the value and set the offset here.
#ifdef BUDDY_PIECES_USE_SPRITES                        
                            TiltNudgePieces(*this, i);
#endif
                        }
                    }
                }
            }
            break;
        }
        case GAME_STATE_FREEPLAY_OPTIONS:
        {   
            if (arraysize(mTouching) > 0 && mTouching[0] == TOUCH_STATE_BEGIN)
            {
                mOptionsTouchSync = true;
                for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
                {
                    mCubeWrappers[i].SetPieceOffset(SIDE_TOP,    Vec2(0, 0));
                    mCubeWrappers[i].SetPieceOffset(SIDE_LEFT,   Vec2(0, 0));
                    mCubeWrappers[i].SetPieceOffset(SIDE_BOTTOM, Vec2(0, 0));
                    mCubeWrappers[i].SetPieceOffset(SIDE_RIGHT,  Vec2(0, 0));
                }
                StartGameState(GAME_STATE_FREEPLAY_PLAY);
            }
            if (arraysize(mTouching) > 1 && mTouching[1] == TOUCH_STATE_BEGIN)
            {
                StartGameState(GAME_STATE_FREEPLAY_START);
            }
            else if (arraysize(mTouching) > 2 && mTouching[2] == TOUCH_STATE_BEGIN)
            {
                StartGameState(GAME_STATE_MAIN_MENU);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_TITLE:
        {
            if (UpdateTimer(mDelayTimer, dt) || AnyTouchBegin())
            {
                StartGameState(GAME_STATE_SHUFFLE_MEMORIZE_FACES);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_MEMORIZE_FACES:
        {
            if (mBackgroundScroll.x > -kMaxTilesX)
            {
                --mBackgroundScroll.x;
            }
            
            if (UpdateTimer(mDelayTimer, dt) || AnyTouchBegin())
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
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    if (mTouching[i] == TOUCH_STATE_BEGIN)
                    {
                        unsigned int newBuddyId = GetRandomOtherBuddyId(*this, mCubeWrappers[i].GetBuddyId());
                        mCubeWrappers[i].SetBuddyId(newBuddyId);
                        
                        ResetCubesToPuzzle(GetPuzzleDefault(), false);
                        
                        if (mUiIndex == 0)
                        {
                            mUiIndexSync[i] = true;
                        }
                    }
                }
            }
            
            if (UpdateTimerLoop(mDelayTimer, dt, kShuffleBannerSwapDelay))
            {
                mUiIndex = (mUiIndex + 1) % 3;
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
                mUiIndex = (mUiIndex + 1) % 2;
            }
            
            if (AnyTouchBegin())
            {
                mOptionsTouchSync = true;
                mClueOffTimers[0] = 0.0f;
                StartGameState(GAME_STATE_SHUFFLE_PLAY);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_PLAY:
        {
            // We're in active play, so track our time for scoring purposes
            mScoreTimer += dt;
            
            // Check for holding, which enables the options menu.
            if (AnyTouchHold())
            {
                if (UpdateTimer(mOptionsTimer, dt))
                {
                    StartGameState(GAME_STATE_SHUFFLE_OPTIONS);
                }
            }
            else
            {
                mOptionsTimer = kOptionsTimerDuration;
            }
            
            // If we're in sprite mode, we can do gradations of nudge, so
            // poll the value and set the offset here.
#ifdef BUDDY_PIECES_USE_SPRITES
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    TiltNudgePieces(*this, i);
                }
            }
#endif
            
            // Check to see if "Face Complete" banners are done displaying
            for (unsigned int i = 0; i < arraysize(mFaceCompleteTimers); ++i)
            {
                if (mFaceCompleteTimers[i] > 0.0f)
                {
                    UpdateTimer(mFaceCompleteTimers[i], dt);
                }
            }
            
            // Check to see if clues are done displaying
            if (mClueOffTimers[0] > 0.0f)
            {
                if (UpdateTimer(mClueOffTimers[0], dt) ||
                    (arraysize(mTouching) > 0 && mTouching[0] == TOUCH_STATE_BEGIN))
                {
                    mClueOffTimers[0] = 0.0f;
                    mClueOnTimer = kClueTimerOnDuration;
                }
            }
            
            if (mOptionsTouchSync)
            {
                if (arraysize(mTouching) > 0 && mTouching[0] == TOUCH_STATE_BEGIN)
                {
                    mOptionsTouchSync = false;
                }
            }
            
            // Check if clue should start being displayed
            if (mClueOnTimer > 0.0f)
            {
                if (UpdateTimer(mClueOnTimer, dt))
                {
                    mClueOffTimers[0] = kStateTimeDelayLong;
                }
            }
            
            // If clues are not displayed, take care of hinting
            if (mClueOffTimers[0] <= 0.0f)
            {
                if (IsHinting())
                {
                    if (mHintTimer > 0.0f && mSwapState == SWAP_STATE_NONE)
                    {
                        if (UpdateTimer(mHintTimer, dt) || AnyTouchBegin())
                        {
                            StopHint(false);
                        }
                    }
                }
                else
                {
                    if (mHintTimer > 0.0f && mSwapState == SWAP_STATE_NONE)
                    {
                        if (UpdateTimer(mHintTimer, dt))
                        {
                            StartHint();
                        }
                    }
                }
            }
            
            break;
        }
        case GAME_STATE_SHUFFLE_OPTIONS:
        {   
            if (arraysize(mTouching) > 0 && mTouching[0] == TOUCH_STATE_BEGIN)
            {
                mOptionsTouchSync = true;
                mClueOffTimers[0] = 0.0f;
                StartGameState(GAME_STATE_SHUFFLE_PLAY);
            }
            if (arraysize(mTouching) > 1 && mTouching[1] == TOUCH_STATE_BEGIN)
            {
                ResetCubesToShuffleStart();
                StartGameState(GAME_STATE_SHUFFLE_UNSHUFFLE_THE_FACES);
            }
            else if (arraysize(mTouching) > 2 && mTouching[2] == TOUCH_STATE_BEGIN)
            {
                StartGameState(GAME_STATE_MAIN_MENU);
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
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    mCubeWrappers[i].UpdateCutscene(
                        kShuffleCutsceneJumpChance, kShuffleCutsceneJumpChance);
                }
            }
            
            if (UpdateTimer(mDelayTimer, dt) || AnyTouchBegin())
            {
                StartGameState(GAME_STATE_SHUFFLE_END_GAME_NAV);
            }
            
            break;
        }
        case GAME_STATE_SHUFFLE_END_GAME_NAV:
        {
            if (mBackgroundScroll.x > -kMaxTilesX)
            {
                --mBackgroundScroll.x;
            }
            
            if (arraysize(mTouching) > 1 && mTouching[1] == TOUCH_STATE_BEGIN)
            {
                StartGameState(GAME_STATE_SHUFFLE_CHARACTER_SPLASH);
            }
            else if (arraysize(mTouching) > 2 && mTouching[2] == TOUCH_STATE_BEGIN)
            {
                StartGameState(GAME_STATE_MAIN_MENU);
            }
            break;
        }
        case GAME_STATE_STORY_CHAPTER_START:
        {
            if (UpdateTimer(mDelayTimer, dt) || AnyTouchBegin())
            {
                StartGameState(GAME_STATE_STORY_CUTSCENE_START);
            }
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_START:
        {
            mCubeWrappers[0].UpdateCutscene(kStoryCutsceneJumpChanceA, kStoryCutsceneJumpChanceB);
            
            if (UpdateTimer(mDelayTimer, dt))
            {
                if (++mStoryCutsceneIndex == GetPuzzle(mStoryPuzzleIndex).GetNumCutsceneTextStart())
                {
                    StartGameState(GAME_STATE_STORY_DISPLAY_START_STATE);
                }
                else
                {
                    mDelayTimer += kStoryCutsceneTextDelay;
                }
            }
            else if (AnyTouchBegin())
            {
                StartGameState(GAME_STATE_STORY_DISPLAY_START_STATE);
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
            if (UpdateTimerLoop(mDelayTimer, dt, kStateTimeDelayLong))
            {
                mUiIndex = mUiIndex == 0 ? 1 : 0;
            }
            
            if (AnyTouchBegin())
            {
                StartGameState(GAME_STATE_STORY_PLAY);
            }
            break;
        }
        case GAME_STATE_STORY_PLAY:
        {
            // We're in active play, so track our time for scoring purposes
            mScoreTimer += dt;
            
            // Check for holding, which enables the options menu.
            if (AnyTouchHold())
            {
                if (UpdateTimer(mOptionsTimer, dt))
                {
                    StartGameState(GAME_STATE_STORY_OPTIONS);
                }
            }
            else
            {
                mOptionsTimer = kOptionsTimerDuration;
            }
            
            // If we're in sprite mode, we can do gradations of nudge, so
            // poll the value and set the offset here.
#ifdef BUDDY_PIECES_USE_SPRITES
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    TiltNudgePieces(*this, i);
                }
            }
#endif
            
            // Check to see if "Face Complete" banners are done displaying
            for (unsigned int i = 0; i < arraysize(mFaceCompleteTimers); ++i)
            {
                if (mFaceCompleteTimers[i] > 0.0f)
                {
                    UpdateTimer(mFaceCompleteTimers[i], dt);
                }
            }
            
            // Check to see if clues are done displaying
            bool clueDisplayed = false;
            for (unsigned int i = 0; i < arraysize(mClueOffTimers); ++i)
            {
                if (mClueOffTimers[i] > 0.0f)
                {
                    clueDisplayed = !UpdateTimer(mClueOffTimers[i], dt);
                }
            }
            
            // If clues are not displayed, see if a hint should be turned on or off
            if (!clueDisplayed)
            {
                if (IsHinting())
                {
                    if (mHintTimer > 0.0f && mSwapState == SWAP_STATE_NONE)
                    {
                        if (UpdateTimer(mHintTimer, dt) || AnyTouchBegin())
                        {
                            StopHint(false);
                        }
                    }
                }
                else
                {
                    if (mHintTimer > 0.0f && mSwapState == SWAP_STATE_NONE)
                    {
                        if (UpdateTimer(mHintTimer, dt))
                        {
                            if (mHintFlowIndex == 0)
                            {
                                ++mHintFlowIndex;
                                mHintTimer = kHintTimerRepeatDuration;
                            }
                            else if (mHintFlowIndex == 1)
                            {
                                ++mHintFlowIndex;
                                mHintTimer = kHintTimerRepeatDuration;
                            }
                            else
                            {
                                StartHint();
                            }
                        }
                    }
                }
            }
            
            // Check if clues should be dismissed
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    if (mTouching[i] == TOUCH_STATE_BEGIN)
                    {
                        if (!mOptionsTouchSync)
                        {
                            if (mClueOffTimers[i] > 0.0f)
                            {
                                mClueOffTimers[i] = 0.0f;
                            }
                            else
                            {
                                mClueOffTimers[i] = kStateTimeDelayLong;
                            }
                        }
                    }
                    else if (mTouching[i] == TOUCH_STATE_END)
                    {
                        mOptionsTouchSync = false;
                    }
                }
            }
            break;
        }
        case GAME_STATE_STORY_OPTIONS:
        {   
            if (arraysize(mTouching) > 0 && mTouching[0] == TOUCH_STATE_BEGIN)
            {
                mOptionsTouchSync = true;
                for (unsigned int i = 0; i < arraysize(mClueOffTimers); ++i)
                {
                    mClueOffTimers[i] = 0.0f;
                }
                StartGameState(GAME_STATE_STORY_PLAY);
            }
            if (arraysize(mTouching) > 1 && mTouching[1] == TOUCH_STATE_BEGIN)
            {
                StartGameState(GAME_STATE_STORY_CHAPTER_START);
            }
            else if (arraysize(mTouching) > 2 && mTouching[2] == TOUCH_STATE_BEGIN)
            {
                StartGameState(GAME_STATE_MAIN_MENU);
            }
            break;
        }
        case GAME_STATE_STORY_SOLVED:
        {
            if (UpdateTimer(mDelayTimer, dt) || AnyTouchBegin())
            {
                StartGameState(GAME_STATE_STORY_CUTSCENE_END_1);
            }
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_END_1:
        {
            if (UpdateTimer(mDelayTimer, dt) || AnyTouchBegin())
            {
                StartGameState(GAME_STATE_STORY_CUTSCENE_END_2);
            }
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_END_2:
        {
            mCubeWrappers[0].UpdateCutscene(kStoryCutsceneJumpChanceA, kStoryCutsceneJumpChanceB);
            
            if (UpdateTimer(mDelayTimer, dt))
            {
                if (++mStoryCutsceneIndex == GetPuzzle(mStoryPuzzleIndex).GetNumCutsceneTextEnd())
                {
                    if (HasUnlocked())
                    {
                        StartGameState(GAME_STATE_STORY_UNLOCKED_1);
                    }
                    else
                    {
                        StartGameState(GAME_STATE_STORY_CHAPTER_END);
                    }
                }
                else
                {
                    mDelayTimer += kStoryCutsceneTextDelay;
                }
            }
            else if (AnyTouchBegin())
            {
                if (HasUnlocked())
                {
                    StartGameState(GAME_STATE_STORY_UNLOCKED_1);
                }
                else
                {
                    StartGameState(GAME_STATE_STORY_CHAPTER_END);
                }
            }
            break;
        }
        case GAME_STATE_STORY_UNLOCKED_1:
        {
            if (mBackgroundScroll.x > -kMaxTilesX)
            {
                --mBackgroundScroll.x;
            }
            else
            {
                StartGameState(GAME_STATE_STORY_UNLOCKED_2);
            }
            break;
        }
        case GAME_STATE_STORY_UNLOCKED_2:
        {
            if (mDelayTimer > 0.0f)
            {
                if (mBackgroundScroll.y < 11)
                {
                    ++mBackgroundScroll.y;
                }
                else
                {
                    UpdateTimer(mDelayTimer, dt);
                }
            }
            else
            {
                if (mBackgroundScroll.y < int(kMaxTilesX + StoryRibbonNewCharacter.height))
                {
                    ++mBackgroundScroll.y;
                }
                else
                {
                    StartGameState(GAME_STATE_STORY_UNLOCKED_3);
                }
            }
            break;
        }
        case GAME_STATE_STORY_UNLOCKED_3:
        {
            if (mDelayTimer > 0.0f)
            {
                if (mBackgroundScroll.y < 14)
                {
                    ++mBackgroundScroll.y;
                }
                else
                {
                    if (UpdateTimer(mDelayTimer, dt))
                    {
                        StartGameState(GAME_STATE_STORY_UNLOCKED_4);
                    }
                    else
                    {
                        for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
                        {
                            if (mCubeWrappers[i].IsEnabled())
                            {
                                // TODO: Make variable
                                mCubeWrappers[i].UpdateCutscene(4, 4);
                            }
                        }
                    }
                }
            }
            break;
        }
        case GAME_STATE_STORY_UNLOCKED_4:
        {
            if (mBackgroundScroll.x < kMaxTilesX)
            {
                ++mBackgroundScroll.x;
            }
            else
            {
                StartGameState(GAME_STATE_STORY_CHAPTER_END);
            }
            break;
        }
        case GAME_STATE_STORY_CHAPTER_END:
        {
            if (arraysize(mTouching) > 0 && mTouching[0] == TOUCH_STATE_BEGIN)
            {
                if (++mStoryPuzzleIndex == GetNumPuzzles())
                {
                    StartGameState(GAME_STATE_MAIN_MENU);
                }
                else
                {
                    StartGameState(GAME_STATE_STORY_CHAPTER_START);
                }
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
        case GAME_STATE_FREEPLAY_PLAY:
        {
            cubeWrapper.DrawBuddy();
            break;
        }
        case GAME_STATE_FREEPLAY_OPTIONS:
        {
            if (cubeWrapper.GetId() == 0 || cubeWrapper.GetId() >  2)
            {
                DrawOption(cubeWrapper, "Resume", false);
            }
            else if (cubeWrapper.GetId() == 1)
            {
                DrawOption(cubeWrapper, "Restart", false);
            }
            else if (cubeWrapper.GetId() == 2)
            {
                DrawOption(cubeWrapper, "Exit", false);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_TITLE:
        {
            cubeWrapper.DrawBackground(ShuffleTitleScreen);
            break;
        }
        case GAME_STATE_SHUFFLE_MEMORIZE_FACES:
        {
            cubeWrapper.DrawBackgroundPartial(
                Vec2(0, 0),
                Vec2(-mBackgroundScroll.x, 0),
                Vec2(kMaxTilesX + mBackgroundScroll.x, kMaxTilesY),
                ShuffleTitleScreen);
            
            cubeWrapper.DrawBackgroundPartial(
                Vec2(kMaxTilesX + mBackgroundScroll.x, 0),
                Vec2(0, 0),
                Vec2(-mBackgroundScroll.x, kMaxTilesY),
                ShuffleMemorizeFaces);
            
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
            
            if (mUiIndex == 0 && !mUiIndexSync[cubeWrapper.GetId()])
            {
                cubeWrapper.DrawUiAsset(Vec2(0, 0), ShuffleTouchToSwap);
            }
            else if (mUiIndex == 1)
            {
                cubeWrapper.DrawUiAsset(Vec2(0, 0), ShuffleShakeToShuffle);
            }
            
            if (mUiIndex != 0)
            {
                mUiIndexSync[cubeWrapper.GetId()] = false;
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
            if (cubeWrapper.GetId() == 0 && mUiIndex == 0)
            {
                cubeWrapper.DrawBackground(ShuffleClueUnscramble);
            }
            else
            {
                cubeWrapper.DrawBuddy();
            }
            break;
        }
        case GAME_STATE_SHUFFLE_PLAY:
        {
            if (cubeWrapper.GetId() < arraysize(mTouching) &&
                mTouching[cubeWrapper.GetId()] &&
                !mOptionsTouchSync)
            {
                cubeWrapper.DrawBackground(GetBuddyFullAsset(cubeWrapper.GetBuddyId()));
            }
            else if (mClueOffTimers[cubeWrapper.GetId()] > 0.0f)
            {
                cubeWrapper.DrawBackground(ShuffleNeighbor);
            }
            else if (mFaceCompleteTimers[cubeWrapper.GetId()] > 0.0f)
            {
                cubeWrapper.DrawBackground(GetBuddyFullAsset(cubeWrapper.GetBuddyId()));
                cubeWrapper.DrawUiAsset(Vec2(0, 0), UiBannerFaceCompleteOrange);
            }
            else
            {
                cubeWrapper.DrawBuddy();
            }
            break;
        }
        case GAME_STATE_SHUFFLE_OPTIONS:
        {
            if (cubeWrapper.GetId() == 0 || cubeWrapper.GetId() >  2)
            {
                DrawOption(cubeWrapper, "Resume", true, mScoreTimer);
            }
            else if (cubeWrapper.GetId() == 1)
            {
                DrawOption(cubeWrapper, "Restart", true, mScoreTimer);
            }
            else if (cubeWrapper.GetId() == 2)
            {
                DrawOption(cubeWrapper, "Exit", true, mScoreTimer);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_SOLVED:
        {
            if (mFaceCompleteTimers[cubeWrapper.GetId()] > 0.0f)
            {
                cubeWrapper.DrawBackground(GetBuddyFullAsset(cubeWrapper.GetBuddyId()));
                cubeWrapper.DrawUiAsset(Vec2(0, 0), UiBannerFaceCompleteOrange);
            }
            else
            {
                cubeWrapper.DrawBuddy();
            }
            break;
        }
        case GAME_STATE_SHUFFLE_CONGRATULATIONS:
        {
            cubeWrapper.DrawCutsceneShuffle(Vec2(0, 0));
            break;
        }
        case GAME_STATE_SHUFFLE_END_GAME_NAV:
        {
            cubeWrapper.DrawCutsceneShuffle(mBackgroundScroll);
            
            if (cubeWrapper.GetId() == 0 || cubeWrapper.GetId() > 2)
            {
                DrawShuffleScore(*this, cubeWrapper, mScoreTimer, mScorePlace, mBackgroundScroll);
            }
            else if (cubeWrapper.GetId() == 1)
            {
                cubeWrapper.DrawBackgroundPartial(
                    Vec2(kMaxTilesX + mBackgroundScroll.x, 0),
                    Vec2(0, 0),
                    Vec2(-mBackgroundScroll.x, kMaxTilesY),
                    ShuffleEndGameNavReplay);
            }
            else if (cubeWrapper.GetId() == 2)
            {
                cubeWrapper.DrawBackgroundPartial(
                    Vec2(kMaxTilesX + mBackgroundScroll.x, 0),
                    Vec2(0, 0),
                    Vec2(-mBackgroundScroll.x, kMaxTilesY),
                    UiEndGameNavExit);
            }
            break;
        }
        case GAME_STATE_STORY_CHAPTER_START:
        {
            DrawStoryChapterTitle(cubeWrapper, mStoryPuzzleIndex);
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_START:
        {
            if (cubeWrapper.GetId() == 0)
            {
                cubeWrapper.DrawCutsceneStory(GetPuzzle(mStoryPuzzleIndex).GetCutsceneTextStart(mStoryCutsceneIndex));
            }
            else
            {
                DrawStoryChapterTitle(cubeWrapper, mStoryPuzzleIndex);
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
                DrawStoryChapterTitle(cubeWrapper, mStoryPuzzleIndex);
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
                DrawStoryChapterTitle(cubeWrapper, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_CLUE:
        {
            if (cubeWrapper.GetId() == 0 && mUiIndex == 0)
            {
                DrawStoryClue(
                    cubeWrapper, 
                    mStoryPuzzleIndex,
                    UiClueBlank,
                    GetPuzzle(mStoryPuzzleIndex).GetClue());
            }
            else if (cubeWrapper.GetId() < GetPuzzle(mStoryPuzzleIndex).GetNumBuddies())
            {
                cubeWrapper.DrawBuddy();
            }
            else
            {
                DrawStoryChapterTitle(cubeWrapper, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_PLAY:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mStoryPuzzleIndex).GetNumBuddies())
            {
                if (mClueOffTimers[cubeWrapper.GetId()] > 0.0f)
                {
                    DrawStoryClue(
                        cubeWrapper,
                        mStoryPuzzleIndex,
                        UiClueBlank,
                        GetPuzzle(mStoryPuzzleIndex).GetClue());
                }
                else if (cubeWrapper.GetId() == 0 && mHintFlowIndex == 1)
                {
                    cubeWrapper.DrawBackground(ShuffleNeighbor);
                }
                else if (mFaceCompleteTimers[cubeWrapper.GetId()] > 0.0f)
                {
                    DrawStoryFaceComplete(cubeWrapper);
                }
                else
                {
                    cubeWrapper.DrawBuddy();
                }
            }
            else
            {
                DrawStoryChapterTitle(cubeWrapper, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_OPTIONS:
        {
            if (cubeWrapper.GetId() == 0 || cubeWrapper.GetId() >  2)
            {
                DrawOption(cubeWrapper, "Resume", true, mScoreTimer);
            }
            else if (cubeWrapper.GetId() == 1)
            {
                DrawOption(cubeWrapper, "Restart", true, mScoreTimer);
            }
            else if (cubeWrapper.GetId() == 2)
            {
                DrawOption(cubeWrapper, "Exit", true, mScoreTimer);
            }
            break;
        }
        case GAME_STATE_STORY_SOLVED:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mStoryPuzzleIndex).GetNumBuddies())
            {
                if (mFaceCompleteTimers[cubeWrapper.GetId()] > 0.0f)
                {
                    DrawStoryFaceComplete(cubeWrapper);
                }
                else
                {
                    cubeWrapper.DrawBuddy();
                }
            }
            else
            {
                DrawStoryChapterTitle(cubeWrapper, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_END_1:
        {
            DrawStoryChapterSummary(cubeWrapper, mStoryPuzzleIndex);
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_END_2:
        {
            if (cubeWrapper.GetId() == 0)
            {
                cubeWrapper.DrawCutsceneStory(GetPuzzle(mStoryPuzzleIndex).GetCutsceneTextEnd(mStoryCutsceneIndex));
            }
            else
            {
                DrawStoryChapterSummary(cubeWrapper, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_UNLOCKED_1:
        {
            // TODO: Put into a function
        
            cubeWrapper.DrawBackgroundPartial(
                Vec2(0, 0),
                Vec2(-mBackgroundScroll.x, 0),
                Vec2(kMaxTilesX + mBackgroundScroll.x, kMaxTilesY),
                UiBackground);
            
            cubeWrapper.DrawBackgroundPartial(
                Vec2(kMaxTilesX + mBackgroundScroll.x, 0),
                Vec2(0, 0),
                Vec2(-mBackgroundScroll.x, kMaxTilesY),
                UiCongratulations);
            
            break;
        }
        case GAME_STATE_STORY_UNLOCKED_2:
        {
            // TODO: cleanup and put in a function for sliding
        
            cubeWrapper.DrawBackground(UiCongratulations);
            
            if (mBackgroundScroll.y > 0 && mBackgroundScroll.y < 20)
            {
                int y = mBackgroundScroll.y - StoryRibbonNewCharacter.height;
                int assetOffset = 0;
                int assetHeight = StoryRibbonNewCharacter.height;
                
                if (mBackgroundScroll.y < int(StoryRibbonNewCharacter.height))
                {
                    y = 0;
                    assetOffset = StoryRibbonNewCharacter.height - mBackgroundScroll.y;
                    assetHeight = mBackgroundScroll.y;
                }
                else if (mBackgroundScroll.y > kMaxTilesY)
                {
                    assetOffset = 0;
                    assetHeight = StoryRibbonNewCharacter.height - (mBackgroundScroll.y - kMaxTilesY);
                }
                
                ASSERT(assetOffset >= 0 && assetOffset <  int(StoryRibbonNewCharacter.height));
                ASSERT(assetHeight >  0 && assetHeight <= int(StoryRibbonNewCharacter.height));
                cubeWrapper.DrawUiAssetPartial(
                    Vec2(0, y),
                    Vec2(0, assetOffset),
                    Vec2(kMaxTilesX, assetHeight),
                    StoryRibbonNewCharacter);
            }
            break;
        }
        case GAME_STATE_STORY_UNLOCKED_3:
        {
            cubeWrapper.DrawBackground(UiCongratulations);
            
            if (mBackgroundScroll.y > 0 && mBackgroundScroll.y < 20)
            {
                int y = mBackgroundScroll.y - UiRibbonGluv.height;
                int assetOffset = 0;
                int assetHeight = UiRibbonGluv.height;
                
                if (mBackgroundScroll.y < int(UiRibbonGluv.height))
                {
                    y = 0;
                    assetOffset = UiRibbonGluv.height - mBackgroundScroll.y;
                    assetHeight = mBackgroundScroll.y;
                }
                else if (mBackgroundScroll.y > kMaxTilesY)
                {
                    assetOffset = 0;
                    assetHeight = UiRibbonGluv.height - (mBackgroundScroll.y - kMaxTilesY);
                }
                
                cubeWrapper.DrawUnlocked3Sprite(mBackgroundScroll);
                
                ASSERT(assetOffset >= 0 && assetOffset <  int(UiRibbonGluv.height));
                ASSERT(assetHeight >  0 && assetHeight <= int(UiRibbonGluv.height));
                cubeWrapper.DrawUiAssetPartial(
                    Vec2(0, y),
                    Vec2(0, assetOffset),
                    Vec2(kMaxTilesX, assetHeight),
                    UiRibbonGluv);
            }
            break;
        }
        case GAME_STATE_STORY_UNLOCKED_4:
        {
            cubeWrapper.DrawBackgroundPartial(
                Vec2(0, 0),
                Vec2(kMaxTilesX - mBackgroundScroll.x, 0),
                Vec2(mBackgroundScroll.x, kMaxTilesY),
                UiBackground);
            
            if (mBackgroundScroll.x < kMaxTilesX)
            {
                cubeWrapper.DrawBackgroundPartial(
                    Vec2(mBackgroundScroll.x, 0),
                    Vec2(0, 0),
                    Vec2(kMaxTilesX - mBackgroundScroll.x, kMaxTilesY),
                    UiCongratulations);
                
                cubeWrapper.DrawUnlocked4Sprite(mBackgroundScroll);
                        
                cubeWrapper.DrawUiAssetPartial(
                    Vec2(mBackgroundScroll.x, 11),
                    Vec2(0, 0),
                    Vec2(kMaxTilesX - mBackgroundScroll.x, UiRibbonGluv.height),
                    UiRibbonGluv);
            }
            break;
        }
        case GAME_STATE_STORY_CHAPTER_END:
        {
            if (cubeWrapper.GetId() == 0)
            {
                if (HasUnlocked())
                {
                    cubeWrapper.DrawBackground(StoryBookStartNext);
                    cubeWrapper.DrawSprite(0, Vec2(32, 14), BuddySpriteFrontGluv);
                }
                else
                {
                    DrawStoryChapterNext(cubeWrapper, mStoryPuzzleIndex);
                }
            }
            else if (cubeWrapper.GetId() == 1)
            {
                DrawStoryChapterRetry(cubeWrapper, mStoryPuzzleIndex);
            }
            else if (cubeWrapper.GetId() == 2)
            {
                cubeWrapper.DrawBackground(UiEndGameNavExit);
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

void App::InsertScore()
{
    for (int i = arraysize(mSaveDataBestTimes) - 1; i >= 0; --i)
    {
        if (mSaveDataBestTimes[i] <= 0.0f || mScoreTimer < mSaveDataBestTimes[i])
        {
            mScorePlace = i;
        }
    }
    
    if (mScorePlace < arraysize(mSaveDataBestTimes))
    {
        for (int i = arraysize(mSaveDataBestTimes) - 1; i > int(mScorePlace); --i)
        {
            mSaveDataBestTimes[i] = mSaveDataBestTimes[i - 1];
        }
        mSaveDataBestTimes[mScorePlace] = mScoreTimer;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::SaveData()
{
#ifdef SIFTEO_SIMULATOR
    DEBUG_LOG(("SaveData = (%u, %.2ff, %.2ff, %.2ff)\n",
        mSaveDataStoryProgress, mSaveDataBestTimes[0], mSaveDataBestTimes[1], mSaveDataBestTimes[1]));
    
    FILE *saveDataFile = std::fopen("SaveData.bin", "wb");
    ASSERT(saveDataFile != NULL);
    
    int numWritten0 =
        std::fwrite(&mSaveDataStoryProgress, sizeof(mSaveDataStoryProgress), 1, saveDataFile);
    ASSERT(numWritten0 == 1);
    
    int numWritten1 =
        std::fwrite(mSaveDataBestTimes, sizeof(mSaveDataBestTimes[0]), arraysize(mSaveDataBestTimes), saveDataFile);
    ASSERT(numWritten1 == arraysize(mSaveDataBestTimes));
    
    int success = std::fclose(saveDataFile);
    ASSERT(success == 0);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::LoadData()
{
#ifdef SIFTEO_SIMULATOR
    if (FILE *saveDataFile = fopen("SaveData.bin", "rb"))
    {
        int success0 = std::fseek(saveDataFile, 0L, SEEK_END);
        ASSERT(success0 == 0);
        
        std::size_t size = std::ftell(saveDataFile);
        
        if (size != (sizeof(mSaveDataStoryProgress) + sizeof(mSaveDataBestTimes)))
        {
            DEBUG_LOG(("SaveData.bin is wrong size. Re-saving..."));
            SaveData();
        }
        else
        {
            int success1 = std::fseek(saveDataFile, 0L, SEEK_SET);
            ASSERT(success1 == 0);
            
            std::size_t numRead0 =
                std::fread(&mSaveDataStoryProgress, sizeof(mSaveDataStoryProgress), 1, saveDataFile);
            ASSERT(numRead0 == 1);
            
            std::size_t numRead1 =
                std::fread(mSaveDataBestTimes, sizeof(mSaveDataBestTimes[0]), arraysize(mSaveDataBestTimes), saveDataFile);
            ASSERT(numRead1 == arraysize(mSaveDataBestTimes));
            
            int success2 = std::fclose(saveDataFile);
            ASSERT(success2 == 0);
            
            DEBUG_LOG(("SaveData = (%u, %.2ff, %.2ff, %.2ff)\n",
                mSaveDataStoryProgress, mSaveDataBestTimes[0], mSaveDataBestTimes[1], mSaveDataBestTimes[1]));
    
        }
    }
#endif
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
        int swapOffset = -kSwapAnimationCount + swap_anim_counter;
        
        if ((mSwapPiece0 % NUM_SIDES) == SIDE_TOP || (mSwapPiece0 % NUM_SIDES) == SIDE_BOTTOM)
        {
            mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPieceOffset(
                mSwapPiece0 % NUM_SIDES,
                Vec2(0, swapOffset));
        }
         else if ((mSwapPiece0 % NUM_SIDES) == SIDE_LEFT || (mSwapPiece0 % NUM_SIDES) == SIDE_RIGHT)
        {
            mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPieceOffset(
                mSwapPiece0 % NUM_SIDES,
                Vec2(swapOffset, 0));
        }
        
        if ((mSwapPiece1 % NUM_SIDES) == SIDE_TOP || (mSwapPiece1 % NUM_SIDES) == SIDE_BOTTOM)
        {        
            mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPieceOffset(
                mSwapPiece1 % NUM_SIDES,
                Vec2(0, swapOffset));
        }
        else if ((mSwapPiece1 % NUM_SIDES) == SIDE_LEFT || (mSwapPiece1 % NUM_SIDES) == SIDE_RIGHT)
        {
            mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPieceOffset(
                mSwapPiece1 % NUM_SIDES,
                Vec2(swapOffset, 0));
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
            
            // Extra 16 frames are for visual overshoot
            float slide_tick = kSwapAnimationSlide / float(kSwapAnimationCount + 16);
            int swap_anim_counter = mSwapAnimationSlideTimer / slide_tick - 16;
            
            int swapOffset = -swap_anim_counter;
            
            if ((mSwapPiece0 % NUM_SIDES) == SIDE_TOP || (mSwapPiece0 % NUM_SIDES) == SIDE_BOTTOM)
            {
                mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPieceOffset(
                    mSwapPiece0 % NUM_SIDES,
                    Vec2(0, swapOffset));
            }
            else if ((mSwapPiece0 % NUM_SIDES) == SIDE_LEFT || (mSwapPiece0 % NUM_SIDES) == SIDE_RIGHT)
            {
                mCubeWrappers[mSwapPiece0 / NUM_SIDES].SetPieceOffset(
                    mSwapPiece0 % NUM_SIDES,
                    Vec2(swapOffset, 0));
            }
            
            if ((mSwapPiece1 % NUM_SIDES) == SIDE_TOP || (mSwapPiece1 % NUM_SIDES) == SIDE_BOTTOM)
            {
                mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPieceOffset(
                    mSwapPiece1 % NUM_SIDES,
                    Vec2(0, swapOffset));
            }
            else if ((mSwapPiece1 % NUM_SIDES) == SIDE_LEFT || (mSwapPiece1 % NUM_SIDES) == SIDE_RIGHT)
            {
                mCubeWrappers[mSwapPiece1 / NUM_SIDES].SetPieceOffset(
                    mSwapPiece1 % NUM_SIDES,
                    Vec2(swapOffset, 0));
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
    
    if (mGameState == GAME_STATE_FREEPLAY_PLAY)
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
            mFaceCompleteTimers[mSwapPiece0 / NUM_SIDES] = kShuffleFaceCompleteTimerDelay;
        }
        
        if (swap1Solved)
        {
            mFaceCompleteTimers[mSwapPiece1 / NUM_SIDES] = kShuffleFaceCompleteTimerDelay;
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
            mFaceCompleteTimers[mSwapPiece0 / NUM_SIDES] = kShuffleFaceCompleteTimerDelay;
        }
        
        if (swap1Solved)
        {
            mFaceCompleteTimers[mSwapPiece1 / NUM_SIDES] = kShuffleFaceCompleteTimerDelay;
        }
        
        if (AllSolved(*this))
        {
            StartGameState(GAME_STATE_STORY_SOLVED);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool App::IsHinting() const
{
    return mHintPiece0 != -1 && mHintPiece1 != -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::ChooseHint()
{
    ASSERT(!IsHinting());

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
    ASSERT(!IsHinting());
    
    ChooseHint();
    
    ASSERT(IsHinting());
    
    mCubeWrappers[mHintPiece0 / NUM_SIDES].StartPieceBlinking(mHintPiece0 % NUM_SIDES);
    mCubeWrappers[mHintPiece1 / NUM_SIDES].StartPieceBlinking(mHintPiece1 % NUM_SIDES);
    
    mHintTimer = kHintTimerOffDuration;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::StopHint(bool reset)
{
    ASSERT(IsHinting());
    
    mCubeWrappers[mHintPiece0 / NUM_SIDES].StopPieceBlinking();
    mCubeWrappers[mHintPiece1 / NUM_SIDES].StopPieceBlinking();
    
    mHintPiece0 = -1;
    mHintPiece1 = -1;
    
    if (reset)
    {
        mHintTimer = kHintTimerOnDuration;
        mHintFlowIndex = 0;
    }
    else
    {
        mHintTimer = kHintTimerRepeatDuration;
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

bool App::AnyTouchHold() const
{
    for (unsigned int i = 0; i < arraysize(mTouching); ++i)
    {
        if (mTouching[i] == TOUCH_STATE_HOLD)
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

bool App::HasUnlocked() const
{
    return
        (mStoryPuzzleIndex + 1) < GetNumPuzzles() &&
        (mStoryPuzzleIndex + 1) == mSaveDataStoryProgress &&
        GetPuzzle(mSaveDataStoryProgress).GetBook() > GetPuzzle(mStoryPuzzleIndex).GetBook();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}
