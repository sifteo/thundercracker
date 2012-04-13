/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "App.h"
#include <limits.h>
#include <sifteo/menu.h>
#include <sifteo/string.h>
#include <sifteo/system.h>
#include "Book.h"
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
// Buddy Assets
// - TODO: Is there a way to automate this using clever Lua scripting?
///////////////////////////////////////////////////////////////////////////////////////////////////

const PinnedAssetImage *kBuddySpritesFront[] =
{
    &BuddySpriteFront0,
    &BuddySpriteFront1,
    &BuddySpriteFront2,
    &BuddySpriteFront3,
    &BuddySpriteFront4,
    &BuddySpriteFront5,
    &BuddySpriteFront6,
    &BuddySpriteFront7,
    &BuddySpriteFront8,
    &BuddySpriteFront9,
    &BuddySpriteFront10,
    &BuddySpriteFront11,
    NULL,
};

const PinnedAssetImage *kBuddySpritesLeft[] =
{
    &BuddySpriteLeft0,
    &BuddySpriteLeft1,
    &BuddySpriteLeft2,
    &BuddySpriteLeft3,
    &BuddySpriteLeft4,
    &BuddySpriteLeft5,
    &BuddySpriteLeft6,
    &BuddySpriteLeft7,
    &BuddySpriteLeft8,
    &BuddySpriteLeft9,
    &BuddySpriteLeft10,
    &BuddySpriteLeft11,
    NULL,
};

const PinnedAssetImage *kBuddySpritesRight[] =
{
    &BuddySpriteRight0,
    &BuddySpriteRight1,
    &BuddySpriteRight2,
    &BuddySpriteRight3,
    &BuddySpriteRight4,
    &BuddySpriteRight5,
    &BuddySpriteRight6,
    &BuddySpriteRight7,
    &BuddySpriteRight8,
    &BuddySpriteRight9,
    &BuddySpriteRight10,
    &BuddySpriteRight11,
    NULL,
};

const AssetImage *kBuddyRibbons[] =
{
    &BuddyRibbon0,
    &BuddyRibbon1,
    &BuddyRibbon2,
    &BuddyRibbon3,
    &BuddyRibbon4,
    &BuddyRibbon5,
    &BuddyRibbon6,
    &BuddyRibbon7,
    &BuddyRibbon8,
    &BuddyRibbon9,
    &BuddyRibbon10,
    &BuddyRibbon11,
    NULL,
};

const AssetImage *kBuddiesFull[] =
{
    &BuddyFull0,
    &BuddyFull1,
    &BuddyFull2,
    &BuddyFull3,
    &BuddyFull4,
    &BuddyFull5,
    &BuddyFull6,
    &BuddyFull7,
    &BuddyFull8,
    &BuddyFull9,
    &BuddyFull10,
    &BuddyFull11,
};

const AssetImage *kBuddiesSmall[] =
{
    &BuddySmall0,
    &BuddySmall1,
    &BuddySmall2,
    &BuddySmall3,
    &BuddySmall4,
    &BuddySmall5,
    &BuddySmall6,
    &BuddySmall7,
    &BuddySmall8,
    &BuddySmall9,
    &BuddySmall10,
    &BuddySmall11,
};

const AssetImage *kStoryBookTitles[] =
{
    &StoryBookTitle0,
    &StoryBookTitle1,
    &StoryBookTitle2,
    &StoryBookTitle3,
    &StoryBookTitle4,
    &StoryBookTitle5,
    &StoryBookTitle6,
    &StoryBookTitle7,
    &StoryBookTitle8,
    &StoryBookTitle9,
    &StoryBookTitle10,
    &StoryBookTitle11,
    &StoryBookTitle12,
    &StoryBookTitle13,
    &StoryBookTitle14,
};

const AssetImage *kStoryCutsceneEnvironments[] =
{
    &Environment_0,
};

const AssetImage *kStoryCutsceneEnvironmentsLeft[] =
{
    &Environment_0_Left,
};

const AssetImage *kStoryCutsceneEnvironmentsRight[] =
{
    &Environment_0_Right,
};

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

int SplitLines(char lines[5][16], int numLines, int numChar, const char *text)
{
    String<128> buffer;
    buffer << text;
    
    int iLine = 0;
    int iChar = 0;
    
    for (int i = 0; i < buffer.size(); ++i)
    {
        if (buffer[i] == '\n')
        {
            ASSERT(iLine < numLines);
            ASSERT(iChar < numChar);
            lines[iLine][iChar] = '\0';
            
            ++iLine;
            iChar = 0;
        }
        else
        {
            ASSERT(iLine < numLines);
            ASSERT(iChar < numChar);
            lines[iLine][iChar++] = buffer[i];
            
            if (i == (buffer.size() - 1))
            {
                ASSERT(iLine < numLines);
                ASSERT(iChar < numChar);
                lines[iLine][iChar] = '\0';
            }
        }
    }
    
    return iLine + 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawShuffleCutscene(CubeWrapper &cubeWrapper, Int2 scroll, BuddyId buddyId, bool spriteJump)
{
    const unsigned int maxTilesX = VidMode::LCD_width / VidMode::TILE;
    const unsigned int maxTilesY = VidMode::LCD_width / VidMode::TILE;
            
    cubeWrapper.DrawBackgroundPartial(
        Vec2(0, 0),
        Vec2(-scroll.x, 0),
        Vec2(maxTilesX + scroll.x, maxTilesY),
        UiCongratulations);
    
    int jump_offset = 8;
    
    cubeWrapper.DrawSprite(
        0,
        Vec2(
            (VidMode::LCD_width / 2) - 32 + (scroll.x * VidMode::TILE),
            spriteJump ?
                VidMode::LCD_height / 2 - 32 :
                VidMode::LCD_height / 2 - 32 + jump_offset),
        *kBuddySpritesFront[buddyId]);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawShuffleScore(
    const App &app,
    CubeWrapper &cubeWrapper,
    float scoreTimer,
    unsigned int place)
{
    // Background
    const AssetImage *backgrounds[] =
    {
        &ShuffleBestTimesHighScore1,
        &ShuffleBestTimesHighScore2,
        &ShuffleBestTimesHighScore3,
    };
    const AssetImage &background = place < arraysize(backgrounds) ? *backgrounds[place] : ShuffleBestTimes;
    
    cubeWrapper.DrawBackground(background);
    
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
            Vec2(4, 4 + (int(i) * 2)),
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
    
        cubeWrapper.DrawUiText(Vec2(3, 11), UiFontWhite, buffer.c_str());
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawStoryBookTitle(CubeWrapper &cubeWrapper, unsigned int bookIndex, unsigned int puzzleIndex)
{
    cubeWrapper.DrawBackground(*kStoryBookTitles[bookIndex]);
    
    ASSERT(bookIndex < GetNumBooks());
    BuddyId buddyId = GetBook(bookIndex).mUnlockBuddyId;
    
    ASSERT(buddyId < arraysize(kBuddySpritesFront));
    cubeWrapper.DrawSprite(
        0,
        Vec2((VidMode::LCD_width / 2) - 32, 20U),
        *kBuddySpritesFront[buddyId]);
    
    String<16> bufferTitle;
    bufferTitle << GetBook(bookIndex).mTitle;
    int xTitle = (kMaxTilesX / 2) - (bufferTitle.size() / 2);
    cubeWrapper.DrawUiText(Vec2(xTitle, 12), UiFontWhite, bufferTitle.c_str());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawStoryChapterTitle(CubeWrapper &cubeWrapper, unsigned int bookIndex, unsigned int puzzleIndex)
{
    cubeWrapper.DrawBackground(StoryChapterTitle);
    
    String<16> bufferChapter;
    bufferChapter << "Chapter " << (puzzleIndex + 1);
    int xChapter = (kMaxTilesX / 2) - (bufferChapter.size() / 2);
    cubeWrapper.DrawUiText(Vec2(xChapter, 6), UiFontHeadingOrange, bufferChapter.c_str());
    
    String<32> bufferTitle;
    bufferTitle << "\"" << GetPuzzle(bookIndex, puzzleIndex).GetTitle() << "\"";
    
    char lines[2][16];
    int numLines = SplitLines(lines, arraysize(lines), arraysize(lines[0]), bufferTitle.c_str());
    
    for (int i = 0; i <= numLines; ++i)
    {
        String<16> s;
        s << lines[i];
        
        int x = (kMaxTilesX / 2) - (s.size() / 2);
        int y = 8 + i * 2;
        cubeWrapper.DrawUiText(Vec2(x, y), UiFontOrange, s.c_str());
    }
    
    if (bufferChapter.size() % 2 != 0)
    {
        cubeWrapper.ScrollUi(Vec2(VidMode::TILE / 2, 0U));
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
    
    char lines[5][16];
    int numLines = SplitLines(lines, arraysize(lines), arraysize(lines[0]), text);
    
    for (int i = 0; i <= numLines; ++i)
    {
        String<16> s;
        s << lines[i];
        
        int x = (kMaxTilesX / 2) - (s.size() / 2);
        int y = 9 - numLines + (i * 2);
        cubeWrapper.DrawUiText(Vec2(x, y), UiFontOrange, s.c_str());
    }
    
    // Scroll over if first line is not even length
    String<16> s;
    s << lines[0];
    if (s.size() % 2 != 0)
    {
        cubeWrapper.ScrollUi(Vec2(VidMode::TILE / 2, 0U));
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
        cubeWrapper.ScrollUi(Vec2(VidMode::TILE / 2, 0U));
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawStoryCutscene(
    CubeWrapper &cubeWrapper,
    unsigned int cutsceneEnvironment,
    const CutsceneLine &line,
    BuddyId buddyId0, BuddyId buddyId1,
    bool jump0, bool jump1)
{
    ASSERT(line.mText != NULL);
    
    if (line.mSpeaker == 0)
    {
        // Background
        ASSERT(cutsceneEnvironment < arraysize(kStoryCutsceneEnvironmentsLeft));
        cubeWrapper.DrawBackground(*kStoryCutsceneEnvironmentsLeft[cutsceneEnvironment]);
        
        // Sprites
        if (line.mView == CutsceneLine::VIEW_RIGHT)
        {
            if (kBuddySpritesRight[buddyId0] != NULL)
            {
                cubeWrapper.DrawSprite(0, Vec2( 0, jump0 ? 60 : 66), *kBuddySpritesRight[buddyId0]);
            }
        }
        else if (line.mView == CutsceneLine::VIEW_LEFT)
        {
            if (kBuddySpritesLeft[buddyId0] != NULL)
            {
                cubeWrapper.DrawSprite(0, Vec2( 0, jump0 ? 60 : 66), *kBuddySpritesLeft[buddyId0]);
            }
        }
        else if (line.mView == CutsceneLine::VIEW_FRONT)
        {
            if (kBuddySpritesFront[buddyId0] != NULL)
            {
                cubeWrapper.DrawSprite(0, Vec2( 0, jump0 ? 60 : 66), *kBuddySpritesFront[buddyId0]);
            }
        }
        
        if (kBuddySpritesLeft[buddyId1] != NULL)
        {
            cubeWrapper.DrawSprite(1, Vec2(64, jump1 ? 60 : 66), *kBuddySpritesLeft[buddyId1]);
        }
        
        // Text
        cubeWrapper.DrawUiText(Vec2(1, 1), UiFontOrange, line.mText);
    }
    else if (line.mSpeaker == 1)
    {
        // Background
        ASSERT(cutsceneEnvironment < arraysize(kStoryCutsceneEnvironmentsRight));
        cubeWrapper.DrawBackground(*kStoryCutsceneEnvironmentsRight[cutsceneEnvironment]);
        
        // Sprites
        if (kBuddySpritesRight[buddyId0] != NULL)
        {
            cubeWrapper.DrawSprite(0, Vec2( 0, jump0 ? 60 : 66), *kBuddySpritesRight[buddyId0]);
        }
        
        if (line.mView == CutsceneLine::VIEW_RIGHT)
        {
            if (kBuddySpritesRight[buddyId1] != NULL)
            {
                cubeWrapper.DrawSprite(1, Vec2(64, jump0 ? 60 : 66), *kBuddySpritesRight[buddyId1]);
            }
        }
        else if (line.mView == CutsceneLine::VIEW_LEFT)
        {
            if (kBuddySpritesLeft[buddyId1] != NULL)
            {
                cubeWrapper.DrawSprite(1, Vec2(64, jump0 ? 60 : 66), *kBuddySpritesLeft[buddyId1]);
            }
        }
        else if (line.mView == CutsceneLine::VIEW_FRONT)
        {
            if (kBuddySpritesFront[buddyId1] != NULL)
            {
                cubeWrapper.DrawSprite(1, Vec2(64, jump0 ? 60 : 66), *kBuddySpritesFront[buddyId1]);
            }
        }
        
        // Text
        cubeWrapper.DrawUiText(Vec2(1, 1), UiFontOrange, line.mText);
    }
    else if (line.mSpeaker == 2)
    {
        // Background
        ASSERT(cutsceneEnvironment < arraysize(kStoryCutsceneEnvironments));
        cubeWrapper.DrawBackground(*kStoryCutsceneEnvironments[cutsceneEnvironment]);
        
        // Sprites
        if (kBuddySpritesRight[buddyId0] != NULL)
        {
            cubeWrapper.DrawSprite(0, Vec2( 0, jump0 ? 60 : 66), *kBuddySpritesRight[buddyId0]);
        }
        
        if (line.mView == CutsceneLine::VIEW_RIGHT)
        {
            if (kBuddySpritesRight[buddyId1] != NULL)
            {
                cubeWrapper.DrawSprite(1, Vec2(64, jump0 ? 60 : 66), *kBuddySpritesRight[buddyId1]);
            }
        }
        else if (line.mView == CutsceneLine::VIEW_LEFT)
        {
            if (kBuddySpritesLeft[buddyId1] != NULL)
            {
                cubeWrapper.DrawSprite(1, Vec2(64, jump0 ? 60 : 66), *kBuddySpritesLeft[buddyId1]);
            }
        }
        else if (line.mView == CutsceneLine::VIEW_FRONT)
        {
            if (kBuddySpritesFront[buddyId1] != NULL)
            {
                cubeWrapper.DrawSprite(1, Vec2(64, jump0 ? 60 : 66), *kBuddySpritesFront[buddyId1]);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawStoryProgress(CubeWrapper &cubeWrapper, unsigned int bookIndex, unsigned int puzzleIndex)
{
    cubeWrapper.DrawBackground(StoryProgress);
    
    BuddyId buddyId = GetBook(bookIndex).mUnlockBuddyId;
    ASSERT(buddyId < arraysize(kBuddiesSmall));
    cubeWrapper.DrawUiAsset(Vec2(2, 2), *kBuddiesSmall[buddyId]);
    
    String<16> buffer;
    buffer << "Chapter " << (puzzleIndex + 1);
    cubeWrapper.DrawUiText( Vec2(5, 2), UiFontHeadingOrangeNoOutline, buffer.c_str());
    cubeWrapper.DrawUiAsset(Vec2(6, 7), StoryProgressNumbers, puzzleIndex + 1);
    cubeWrapper.DrawUiAsset(Vec2(8, 7), StoryProgressNumbers, GetBook(bookIndex).mNumPuzzles);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawUnlocked3Sprite(CubeWrapper &cubeWrapper, BuddyId buddyId, Int2 scroll, bool jump)
{
    int jump_offset = 4;
    
    int x = (VidMode::LCD_width / 2) - 32 + (scroll.x * VidMode::TILE);
    int y = jump ? 28 - jump_offset : 28;
    y += -VidMode::LCD_height + ((scroll.y + 2) * VidMode::TILE); // TODO: +2 is fudge, refactor
    
    ASSERT(buddyId < arraysize(kBuddySpritesFront));
    cubeWrapper.DrawSprite(0, Vec2(x, y), *kBuddySpritesFront[buddyId]);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawUnlocked4Sprite(CubeWrapper &cubeWrapper, BuddyId buddyId, Int2 scroll)
{
    ASSERT(buddyId < arraysize(kBuddySpritesFront));
    cubeWrapper.DrawSprite(
        0,
        Vec2((VidMode::LCD_width / 2) - 32 + (scroll.x * VidMode::TILE), 28U),
        *kBuddySpritesFront[buddyId]);
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

bool IsBuddyUsed(App &app, BuddyId buddyId)
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

int GetRotationTarget(const Piece &piece, Cube::Side side)
{
    int rotation = side - piece.GetPart();
    if (rotation < 0)
    {
        rotation += NUM_SIDES;
    }
    
    return rotation;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TODO: kill this in favor of just using the above
///////////////////////////////////////////////////////////////////////////////////////////////////

bool IsAtRotationTarget(const Piece &piece, Cube::Side side)
{
    return piece.GetRotation() == GetRotationTarget(piece, side);
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

BuddyId GetRandomOtherBuddyId(App &app, BuddyId buddyId)
{
    int numBuddies = NUM_BUDDIES - 1; // Don't allow invisible buddy!
    
    Random random;
    int selection = random.randrange(1U, numBuddies - kNumCubes);
    
    for (int j = 0; j < selection; ++j)
    {
        buddyId = BuddyId((buddyId + 1) % numBuddies);
        
        while (IsBuddyUsed(app, buddyId))
        {
            buddyId = BuddyId((buddyId + 1) % numBuddies);
        }
    }
    
    return buddyId;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: Get this wrapped only in a DEBUG build
const char *kGameStateNames[NUM_GAME_STATES] =
{
    "GAME_STATE_NONE",
    "GAME_STATE_TITLE",
    "GAME_STATE_MENU_MAIN",
    "GAME_STATE_MENU_STORY",
    "GAME_STATE_FREEPLAY_START",
    "GAME_STATE_FREEPLAY_TITLE",
    "GAME_STATE_FREEPLAY_DESCRIPTION",
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
    "GAME_STATE_STORY_BOOK_START",
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
    "GAME_STATE_STORY_CHAPTER_END",
    "GAME_STATE_STORY_GAME_END_CONGRATS",
    "GAME_STATE_STORY_GAME_END_MORE",
    "GAME_STATE_UNLOCKED_1",
    "GAME_STATE_UNLOCKED_2",
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
    , mUiIndex(0)
    , mUiIndexSync()
    , mTouching()
    , mTouchSync(false)
    , mTouchEndChoiceTimer(0.0f)
    , mTouchEndChoice(-1)
    , mScoreTimer(0.0f)
    , mScoreMoves(0)
    , mScorePlace(UINT_MAX)
    , mSaveDataStoryBookProgress(0)
    , mSaveDataStoryPuzzleProgress(0)
    , mSaveDataBuddyUnlockMask(0)
    , mSaveDataBestTimes()
    , mSwapState(SWAP_STATE_NONE)
    , mSwapPiece0(0)
    , mSwapPiece1(0)
    , mSwapAnimationSlideTimer(0)
    , mSwapAnimationRotateTimer(0.0f)
    , mFaceCompleteTimers()
    , mBackgroundScroll(Vec2(0, 0))
    , mHintTimer(0.0f)
    , mHintPiece0(-1)
    , mHintPiece1(-1)
    , mHintPieceSkip(-1)
    , mHintFlowIndex(0)
    , mClueOffTimers()
    , mFreePlayShakeThrottleTimer(0.0f)
    , mShufflePiecesStart()
    , mShuffleMoveCounter(0)
    , mStoryPreGame(false)
    , mStoryBookIndex(0)
    , mStoryPuzzleIndex(0)
    , mStoryBuddyUnlockMask(0)
    , mStoryCutsceneIndex(0)
    , mCutsceneSpriteJumpRandom()
    , mCutsceneSpriteJump0(false)
    , mCutsceneSpriteJump1(false)
{
    for (unsigned int i = 0; i < arraysize(mTouching); ++i)
    {
        mTouching[i] = TOUCH_STATE_NONE;
    }
    
    mSaveDataBestTimes[0] = 0.0f;
    mSaveDataBestTimes[1] = 0.0f;
    mSaveDataBestTimes[2] = 0.0f;
    
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
    
    LoadData();
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
    static bool skipDt = false;
    if (skipDt)
    {
        dt = 0.0f;
        skipDt = false;
    }
    
    if (mGameState == GAME_STATE_MENU_MAIN)
    {
        UpdateMenuMain();
        skipDt = true;
    }
    else if (mGameState == GAME_STATE_MENU_STORY)
    {
        UpdateMenuStory();
        skipDt = true;
    }
    else
    {
        UpdateTouch();
        UpdateGameState(dt);
        UpdateSwap(dt);
        UpdateCubes(dt);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::Draw()
{
    if (mGameState != GAME_STATE_MENU_MAIN && mGameState != GAME_STATE_MENU_STORY)
    {   
        DrawGameState();
        
        if (NeedPaintSync(*this))
        {
            System::paintSync();
        }
        System::paint();
    }
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
        
        if (mGameState == GAME_STATE_SHUFFLE_PLAY || mGameState == GAME_STATE_STORY_PLAY)
        {
            mDelayTimer = 0.0f; // Turn off "GO!" sprite
            mHintTimer = kHintTimerOnDuration; // In case neighbor happens between hinting cycles
            mHintFlowIndex = 0;
        }
        
        for (unsigned int i = 0; i < arraysize(mClueOffTimers); ++i)
        {
            mClueOffTimers[i] = 0.0f;
        }
        
        GetCubeWrapper(cubeId0).StartBump(cubeSide0);
        GetCubeWrapper(cubeId1).StartBump(cubeSide1);
        
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
            (   cubeId0 < GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetNumBuddies() &&
                cubeId1 < GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetNumBuddies());
        
        if (isValidGameState)
        {
            if (!isSwapping && !isFixed && isValidCube)
            {
                ++mScoreMoves;
                OnSwapBegin(cubeId0 * NUM_SIDES + cubeSide0, cubeId1 * NUM_SIDES + cubeSide1);
            }
            else
            {
                PlaySound(SoundSwapFail);
            }
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
            Cube::TiltState tiltState = GetCubeWrapper(cubeId).GetTiltState();
            if (tiltState.x == 0 || tiltState.y == 0 || tiltState.x == 2 || tiltState.y == 2)
            {
                PlaySound(SoundPieceNudge);
            }
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
                Cube::TiltState tiltState = GetCubeWrapper(cubeId).GetTiltState();
                if (tiltState.x == 0 || tiltState.y == 0 || tiltState.x == 2 || tiltState.y == 2)
                {
                    PlaySound(SoundPieceNudge);
                }
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
        case GAME_STATE_STORY_CLUE:
        {
            StartGameState(GAME_STATE_STORY_PLAY);
            break;
        }
        case GAME_STATE_STORY_PLAY:
        {
            if (mSwapState == SWAP_STATE_NONE)
            {
                Cube::TiltState tiltState = GetCubeWrapper(cubeId).GetTiltState();
                if (tiltState.x == 0 || tiltState.y == 0 || tiltState.x == 2 || tiltState.y == 2)
                {
                    PlaySound(SoundPieceNudge);
                }
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
                PlaySound(SoundFreePlayReset);
                
                mFreePlayShakeThrottleTimer = kFreePlayShakeThrottleDuration;
            
                BuddyId newBuddyId =
                    GetRandomOtherBuddyId(*this, mCubeWrappers[cubeId].GetBuddyId());
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
                    Piece pieceStart = puzzle.GetPieceStart(i, j);
                    pieceStart.SetRotation(GetRotationTarget(pieceStart, j));
                    mCubeWrappers[i].SetPiece(j, pieceStart);
                    
                    Piece pieceEnd = puzzle.GetPieceEnd(i, j);
                    pieceEnd.SetRotation(GetRotationTarget(pieceEnd, j));
                    mCubeWrappers[i].SetPieceSolution(j, pieceEnd);
                }
            }
            else
            {
                // TODO: This is definitely hacky... instead of igoring the active puzzle,
                // the Free Play shake shuffle should alter the current Puzzle. But then we'd
                // need to keep an editable copy of Puzzle around...
                for (unsigned int j = 0; j < NUM_SIDES; ++j)
                {
                    BuddyId buddyId = mCubeWrappers[i].GetBuddyId();
                    
                    Piece pieceStart = puzzle.GetPieceStart(buddyId, j);
                    pieceStart.SetRotation(GetRotationTarget(pieceStart, j));
                    mCubeWrappers[i].SetPiece(j, pieceStart);
                    
                    Piece pieceEnd = puzzle.GetPieceEnd(buddyId, j);
                    pieceEnd.SetRotation(GetRotationTarget(pieceEnd, j));
                    mCubeWrappers[i].SetPieceSolution(j, pieceEnd);
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
    bool blinked = false;
    
    for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
    {
        if (mCubeWrappers[i].IsEnabled())
        {
            bool blinkedCube = mCubeWrappers[i].Update(dt);
            blinked = blinked || blinkedCube;
        }
    }
    
    if (blinked)
    {
        PlaySound(SoundHintBlink);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::PlaySound(const Sifteo::AssetAudio &audioAsset)
{
    mChannel.play(audioAsset);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::UpdateMenuMain()
{
    MenuAssets menuAssets =
    {
        &BgTile,
        &Footer,
        &LabelEmpty,
        { &TipSelectMode, &TipTiltToScroll, &TipPressToSelect, NULL },
    };
    
    MenuItem menuItems[] =
    {
        { &IconStory,    &LabelStory },
        { &IconShuffle,  &LabelShuffle },
        { &IconFreePlay, &LabelFreePlay },
      //{ &IconOptions,  &LabelOptions }, // TODO: Disabled until we actually implement it
        { NULL,          NULL },
    };
    
    const AssetImage *kMenuNeighborAssets[] =
    {
        &MenuNeighborStory,
        &MenuNeighborShuffle,
        &MenuNeighborFreePlay,
        &MenuNeighborMesssage,
    };
    
    int item = -1;
    
    int menuNeighborIndices[kNumCubes];
    for (int i = 0; i < arraysize(menuNeighborIndices); ++i)
    {
        menuNeighborIndices[i] = arraysize(kMenuNeighborAssets) - 1;
    }
    
    bool neighbored[kNumCubes];
    for (int i = 0; i < arraysize(neighbored); ++i)
    {
        neighbored[i] = false;
    }
    
    Menu menu(&mCubeWrappers[0].GetCube(), &menuAssets, menuItems);
    menu.setIconYOffset(32);
    
    // TODO: Sound
    
    MenuEvent menuEvent;
    while (menu.pollEvent(&menuEvent))
    {
        switch (menuEvent.type)
        {
            case MENU_ITEM_PRESS:
            {
                if (menuEvent.item == 0)
                {
                    StartGameState(GAME_STATE_MENU_STORY);
                }
                else if (menuEvent.item == 1)
                {
                    StartGameState(GAME_STATE_SHUFFLE_START);
                }
                else if (menuEvent.item == 2)
                {
                    StartGameState(GAME_STATE_FREEPLAY_START);
                }
                else if (menuEvent.item == 3)
                {
                    // TODO: options config menu
                    menu.preventDefault();
                }
                break;
            }
            case MENU_NEIGHBOR_ADD:
            {
                ASSERT(item >= 0 && item < arraysize(kMenuNeighborAssets));
                ASSERT(menuEvent.neighbor.neighbor < arraysize(menuNeighborIndices));
                menuNeighborIndices[menuEvent.neighbor.neighbor] = item;
                
                ASSERT(menuEvent.neighbor.neighbor < arraysize(neighbored));
                neighbored[menuEvent.neighbor.neighbor] = true;
                break;
            }
            case MENU_NEIGHBOR_REMOVE:
            {
                ASSERT(menuEvent.neighbor.neighbor < arraysize(menuNeighborIndices));
                menuNeighborIndices[menuEvent.neighbor.neighbor] = arraysize(kMenuNeighborAssets) - 1;
                
                ASSERT(menuEvent.neighbor.neighbor < arraysize(neighbored));
                neighbored[menuEvent.neighbor.neighbor] = false;
                break;
            }
            case MENU_ITEM_ARRIVE:
            {
                item = menuEvent.item;
                
                for (int i = 0; i < arraysize(menuNeighborIndices); ++i)
                {
                    if (neighbored[i])
                    {
                        menuNeighborIndices[i] = item;
                    }
                }
                break;
            }
            case MENU_ITEM_DEPART:
            {
                item = -1;
                
                for (int i = 0; i < arraysize(menuNeighborIndices); ++i)
                {
                    if (neighbored[i])
                    {
                        menuNeighborIndices[i] = arraysize(kMenuNeighborAssets) - 1;
                    }
                }
                break;
            }
            case MENU_PREPAINT:
            {
                for (int i = 1; i < arraysize(mCubeWrappers); ++i)
                {
                    if (mCubeWrappers[i].IsEnabled())
                    {
                        mCubeWrappers[i].DrawBackground(*kMenuNeighborAssets[menuNeighborIndices[i]]);
                        mCubeWrappers[i].DrawUiAsset(Vec2(0, 0), LabelEmpty);
                        mCubeWrappers[i].DrawUiAsset(Vec2(0, 14), Footer);
                        mCubeWrappers[i].DrawFlush();
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
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::UpdateMenuStory()
{
    MenuAssets menuAssets =
    {
        &BgTile,
        &Footer,
        &LabelEmpty,
        { &TipSelectBook, &TipTiltToScroll, &TipPressToSelect, NULL },
    };
    MenuItem menuItems[] =
    {
        { &IconContinue, &LabelContinue },
        { &IconBook0,    &LabelBook0 },
        { &IconBook1,    &LabelBook1 },
        { &IconBook2,    &LabelBook2 },
        { &IconBack,     &LabelBack },
        { NULL,          NULL },
    };
    
    const AssetImage *kMenuNeighborAssets[] =
    {
        &MenuNeighborMesssage,
        &MenuNeighborBook1,
        &MenuNeighborBook2,
        &MenuNeighborBook3,
        &MenuNeighborMesssage,
        &MenuNeighborMesssage,
    };
    
    int item = -1;
    int menuNeighborIndices[kNumCubes];
    for (int i = 0; i < arraysize(menuNeighborIndices); ++i)
    {
        menuNeighborIndices[i] = arraysize(kMenuNeighborAssets) - 1;
    }
    
    bool neighbored[kNumCubes];
    for (int i = 0; i < arraysize(neighbored); ++i)
    {
        neighbored[i] = false;
    }
    
    Menu menu(&mCubeWrappers[0].GetCube(), &menuAssets, menuItems);
    menu.setIconYOffset(32);
    
    // TODO: Sound
    
    MenuEvent menuEvent;
    while (menu.pollEvent(&menuEvent))
    {
        switch (menuEvent.type)
        {
            case MENU_ITEM_PRESS:
            {
                if (menuEvent.item == 0)
                {
                    mStoryBookIndex = mSaveDataStoryBookProgress;
                    mStoryPuzzleIndex = mSaveDataStoryPuzzleProgress;
                    StartGameState(GAME_STATE_STORY_START);
                }
                else if (menuEvent.item >= 1 && menuEvent.item <= 3)
                {
                    mStoryBookIndex = menuEvent.item - 1;
                    mStoryPuzzleIndex = 0;
                    StartGameState(GAME_STATE_STORY_START);
                }
                else if (menuEvent.item == 4)
                {
                    StartGameState(GAME_STATE_MENU_MAIN);
                }
                break;
            }
            case MENU_NEIGHBOR_ADD:
            {
                ASSERT(item >= 0 && item < arraysize(kMenuNeighborAssets));
                ASSERT(menuEvent.neighbor.neighbor < arraysize(menuNeighborIndices));
                menuNeighborIndices[menuEvent.neighbor.neighbor] = item;
                
                ASSERT(menuEvent.neighbor.neighbor < arraysize(neighbored));
                neighbored[menuEvent.neighbor.neighbor] = true;
                break;
            }
            case MENU_NEIGHBOR_REMOVE:
            {
                ASSERT(menuEvent.neighbor.neighbor < arraysize(menuNeighborIndices));
                menuNeighborIndices[menuEvent.neighbor.neighbor] = arraysize(kMenuNeighborAssets) - 1;
                
                ASSERT(menuEvent.neighbor.neighbor < arraysize(neighbored));
                neighbored[menuEvent.neighbor.neighbor] = false;
                break;
            }
            case MENU_ITEM_ARRIVE:
            {
                item = menuEvent.item;
                
                for (int i = 0; i < arraysize(menuNeighborIndices); ++i)
                {
                    if (neighbored[i])
                    {
                        menuNeighborIndices[i] = item;
                    }
                }
                break;
            }
            case MENU_ITEM_DEPART:
            {
                item = -1;
                
                for (int i = 0; i < arraysize(menuNeighborIndices); ++i)
                {
                    if (neighbored[i])
                    {
                        menuNeighborIndices[i] = arraysize(kMenuNeighborAssets) - 1;
                    }
                }
                break;
            }
            case MENU_PREPAINT:
            {
                for (int i = 1; i < arraysize(mCubeWrappers); ++i)
                {
                    if (mCubeWrappers[i].IsEnabled())
                    {
                        mCubeWrappers[i].DrawBackground(*kMenuNeighborAssets[menuNeighborIndices[i]]);
                        mCubeWrappers[i].DrawUiAsset(Vec2(0, 0), LabelEmpty);
                        mCubeWrappers[i].DrawUiAsset(Vec2(0, 14), Footer);
                        mCubeWrappers[i].DrawFlush();
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
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::StartGameState(GameState gameState)
{
    mGameState = gameState;
    
    ASSERT(gameState < int(arraysize(kGameStateNames)));  
    LOG(("Game State = %s\n", kGameStateNames[mGameState]));
    
    switch (mGameState)
    {
        case GAME_STATE_TITLE:
        {
            mDelayTimer = kStateTimeDelayLong;
            break;
        }
        case GAME_STATE_MENU_MAIN:
        {
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    mCubeWrappers[i].DrawClear();
                    
                    if (i > 0)
                    {
                        mCubeWrappers[i].DrawBackground(UiTitleScreen);
                    }
                }
            }
            break;
        }
        case GAME_STATE_MENU_STORY:
        {
            // TODO: Jump to book menu item based on unlock flow
            
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    mCubeWrappers[i].DrawClear();
                    
                    if (i > 0)
                    {
                        mCubeWrappers[i].DrawBackground(UiTitleScreen);
                    }
                }
            }
            break;
        }
        case GAME_STATE_FREEPLAY_START:
        {
            unsigned int buddyIds[NUM_BUDDIES - 1];  // Don't allow invisible buddy!
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
                mCubeWrappers[i].SetBuddyId(BuddyId(buddyIds[i]));
            }
            
            ResetCubesToPuzzle(GetPuzzleDefault(), false);
            StartGameState(GAME_STATE_FREEPLAY_TITLE);
            break;
        }
        case GAME_STATE_FREEPLAY_TITLE:
        {
            mDelayTimer = kModeTitleDelay;
            break;
        }
        case GAME_STATE_FREEPLAY_DESCRIPTION:
        {
            mDelayTimer = kStateTimeDelayLong;
            break;
        }
        case GAME_STATE_FREEPLAY_PLAY:
        {
            mOptionsTimer = kOptionsTimerDuration;
            mFreePlayShakeThrottleTimer = 0.0f;
            break;
        }
        case GAME_STATE_FREEPLAY_OPTIONS:
        {
            PlaySound(SoundPause);
            mTouchEndChoiceTimer = 0.0f;
            mTouchEndChoice = -1;
            break;
        }
        case GAME_STATE_SHUFFLE_START:
        {
            unsigned int buddyIds[NUM_BUDDIES - 1];  // Don't allow invisible buddy!
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
                mCubeWrappers[i].SetBuddyId(BuddyId(buddyIds[i]));
            }
            
            ResetCubesToPuzzle(GetPuzzleDefault(), false);
            StartGameState(GAME_STATE_SHUFFLE_TITLE);
            break;
        }
        case GAME_STATE_SHUFFLE_TITLE:
        {
            mDelayTimer = kModeTitleDelay;
            break;
        }
        case GAME_STATE_SHUFFLE_MEMORIZE_FACES:
        {
            mDelayTimer = kStateTimeDelayLong;
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
            for (unsigned int i = 0; i < arraysize(mUiIndexSync); ++i)
            {
                mUiIndexSync[i] = false;
            }
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
            PlaySound(SoundShuffleBegin);
            break;
        }
        case GAME_STATE_SHUFFLE_UNSHUFFLE_THE_FACES:
        {
            mDelayTimer = kStateTimeDelayLong;
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
            mDelayTimer = kStateTimeDelayShort;
            mOptionsTimer = kOptionsTimerDuration;
            mScoreTimer = 0.0f;
            mScoreMoves = 0;
            mScorePlace = UINT_MAX;
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
        case GAME_STATE_SHUFFLE_OPTIONS:
        {
            PlaySound(SoundPause);
            mTouchEndChoiceTimer = 0.0f;
            mTouchEndChoice = -1;
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
            mCutsceneSpriteJump0 = false;
            break;
        }
        case GAME_STATE_SHUFFLE_END_GAME_NAV:
        {
            mTouchEndChoiceTimer = 0.0f;
            mTouchEndChoice = -1;
            break;
        }
        case GAME_STATE_STORY_START:
        {
            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
            {
                if (mCubeWrappers[i].IsEnabled())
                {
                    mCubeWrappers[i].SetBuddyId(BuddyId(i % NUM_BUDDIES));
                }
            }
            StartGameState(GAME_STATE_STORY_BOOK_START);
            break;
        }
        case GAME_STATE_STORY_BOOK_START:
        {
            mDelayTimer = kModeTitleDelay;
            break;
        }
        case GAME_STATE_STORY_CHAPTER_START:
        {
            ASSERT(mStoryPuzzleIndex < GetBook(mStoryBookIndex).mNumPuzzles);
            ResetCubesToPuzzle(GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex), true);
            mDelayTimer = kModeTitleDelay;
            PlaySound(SoundStoryChapterTitle);
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_START:
        {
            if (GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetNumCutsceneLineStart() == 1)
            {
                mDelayTimer = kStateTimeDelayLong;
            }
            else
            {
                mDelayTimer = kStoryCutsceneTextDelay;
            }
            mStoryCutsceneIndex = 0;
            mCutsceneSpriteJump0 = false;
            mCutsceneSpriteJump1 = false;
            PlaySound(SoundCutsceneChatter);
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
            ShufflePieces(GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetNumBuddies());
            PlaySound(SoundShuffleBegin);
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
            mDelayTimer = kStateTimeDelayShort; // Turn on GO! sprite
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
            PlaySound(SoundGameStart);
            break;
        }
        case GAME_STATE_STORY_OPTIONS:
        {
            PlaySound(SoundPause);
            mTouchEndChoiceTimer = 0.0f;
            mTouchEndChoice = -1;
            break;
        }
        case GAME_STATE_STORY_SOLVED:
        {
            mUiIndex = 0;
            mDelayTimer = kStateTimeDelayShort;
            
            // TODO: Ideally we'd do the actual save the moment the player solves
            // the puzzle. But this is causing bugs with the unlocked logic,
            // so I'm moving it to happen alongside mStoryBookIndex/mStoryPuzzleIndex
            // for now.
            /*
            if ((mStoryPuzzleIndex + 1) == GetNumPuzzles(mStoryBookIndex))
            {
                if ((mStoryBookIndex + 1) > mSaveDataStoryBookProgress)
                {
                    mSaveDataStoryBookProgress = mStoryBookIndex + 1;
                    mSaveDataStoryPuzzleProgress = 0;
                    SaveData();
                }
            } 
            else
            {
                if ((mStoryPuzzleIndex + 1) > mSaveDataStoryPuzzleProgress)
                {
                    mSaveDataStoryPuzzleProgress = mStoryPuzzleIndex + 1;
                    SaveData();
                }
            }
            */
            
            // Are there are any new buddies to unlock?
            if ((mStoryPuzzleIndex + 1) == GetBook(mStoryBookIndex).mNumPuzzles)
            {
                if ((mStoryBookIndex + 1) < GetNumBooks())
                {
                    mSaveDataBuddyUnlockMask |= 1 << GetBook(mStoryBookIndex + 1).mUnlockBuddyId;
                }
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
            if (GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetNumCutsceneLineEnd() == 1)
            {
                mDelayTimer = kStateTimeDelayLong;
            }
            else
            {
                mDelayTimer = kStoryCutsceneTextDelay;
            }
            mStoryCutsceneIndex = 0;
            mCutsceneSpriteJump0 = false;
            mCutsceneSpriteJump1 = false;
            break;
        }
        case GAME_STATE_STORY_CHAPTER_END:
        {
            mTouchEndChoiceTimer = 0.0f;
            mTouchEndChoice = -1;
            break;
        }
        case GAME_STATE_UNLOCKED_1:
        {
            mDelayTimer = kStateTimeDelayLong;
            mBackgroundScroll = Vec2(0, 0);
            break;
        }
        case GAME_STATE_UNLOCKED_2:
        {
            mDelayTimer = kStateTimeDelayLong;
            mBackgroundScroll = Vec2(0, 0);
            mCutsceneSpriteJump0 = false;
            break;
        }
        case GAME_STATE_STORY_GAME_END_CONGRATS:
        {
            mDelayTimer = kStateTimeDelayLong;
            break;
        }
        case GAME_STATE_STORY_GAME_END_MORE:
        {
            mDelayTimer = kStateTimeDelayLong;
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
        case GAME_STATE_TITLE:
        {
            if (UpdateTimer(mDelayTimer, dt) || AnyTouchBegin())
            {
                // TODO: Disabled for Alpha build
#if 0
                if (NextUnlockedBuddy() != -1)
                {
                    mStoryPreGame = true;
                    StartGameState(GAME_STATE_UNLOCKED_1);
                }
                else
                {
                    
                    StartGameState(GAME_STATE_MENU_MAIN);
                }
#else
                mStoryBuddyUnlockMask |= mSaveDataBuddyUnlockMask;
                {
                    
                    StartGameState(GAME_STATE_MENU_MAIN);
                }
#endif
            }
            break;
        }
        case GAME_STATE_FREEPLAY_TITLE:
        {
            if (UpdateTimer(mDelayTimer, dt) || AnyTouchBegin())
            {
                StartGameState(GAME_STATE_FREEPLAY_DESCRIPTION);
            }
            break;
        }
        case GAME_STATE_FREEPLAY_DESCRIPTION:
        {
            if (UpdateTimer(mDelayTimer, dt) || AnyTouchBegin())
            {
                mTouchSync = true;
                StartGameState(GAME_STATE_FREEPLAY_PLAY);
            }
            break;
        }
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
                        mTouchSync = true;
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
                            if (!mTouchSync)
                            {
                                if (mTouching[i] == TOUCH_STATE_BEGIN)
                                {
                                    PlaySound(SoundPiecePinch);
                                }
                                mCubeWrappers[i].SetPieceOffset(SIDE_TOP,    Vec2(0U, VidMode::TILE));
                                mCubeWrappers[i].SetPieceOffset(SIDE_LEFT,   Vec2(VidMode::TILE, 0U));
                                mCubeWrappers[i].SetPieceOffset(SIDE_BOTTOM, Vec2(0U, VidMode::TILE));
                                mCubeWrappers[i].SetPieceOffset(SIDE_RIGHT,  Vec2(VidMode::TILE, 0U));
                            }
                        }
                        else if (mTouching[i] == TOUCH_STATE_END)
                        {
                            mTouchSync = false;
                            
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
        case GAME_STATE_FREEPLAY_OPTIONS:
        {   
            if (mTouchSync)
            {
                for (int i = 0; i < arraysize(mTouching); ++i)
                {
                    if (mTouching[i] == TOUCH_STATE_BEGIN)
                    {
                        mTouchSync = false;
                    }
                }
            }
            else
            {
                if (mTouchEndChoiceTimer > 0.0f)
                {
                    if (UpdateTimer(mTouchEndChoiceTimer, dt))
                    {
                        ASSERT(mTouchEndChoice >= 0 && mTouchEndChoice < 3);
                        if (mTouchEndChoice == 0)
                        {
                            for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
                            {
                                mCubeWrappers[i].SetPieceOffset(SIDE_TOP,    Vec2(0, 0));
                                mCubeWrappers[i].SetPieceOffset(SIDE_LEFT,   Vec2(0, 0));
                                mCubeWrappers[i].SetPieceOffset(SIDE_BOTTOM, Vec2(0, 0));
                                mCubeWrappers[i].SetPieceOffset(SIDE_RIGHT,  Vec2(0, 0));
                            }
                            PlaySound(SoundUnpause);
                            StartGameState(GAME_STATE_FREEPLAY_PLAY);
                        }
                        else if (mTouchEndChoice == 1)
                        {
                            ResetCubesToPuzzle(GetPuzzleDefault(), false);
                            StartGameState(GAME_STATE_FREEPLAY_PLAY);
                        }
                        else if (mTouchEndChoice == 2)
                        {
                            StartGameState(GAME_STATE_MENU_MAIN);
                        }
                    }
                }
                else
                {
                    for (int i = 0; i < 3; ++i)
                    {
                        if (mTouching[i] == TOUCH_STATE_END)
                        {
                            mTouchEndChoiceTimer = kPushButtonDelay;
                            mTouchEndChoice = i;
                            break;
                        }
                    }
                }
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
                        BuddyId newBuddyId =
                            GetRandomOtherBuddyId(*this, mCubeWrappers[i].GetBuddyId());
                        mCubeWrappers[i].SetBuddyId(newBuddyId);
                        
                        ResetCubesToPuzzle(GetPuzzleDefault(), false);
                        
                        mUiIndexSync[i] = true;
                    }
                }
            }
            
            if (UpdateTimerLoop(mDelayTimer, dt, kShuffleBannerSwapDelay))
            {
                mUiIndex = (mUiIndex + 1) % 3;
                
                for (unsigned int i = 0; i < arraysize(mUiIndexSync); ++i)
                {
                    mUiIndexSync[i] = false;
                }
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
            if (UpdateTimer(mDelayTimer, dt))
            {
                StartGameState(GAME_STATE_SHUFFLE_PLAY);
            }
            else if (AnyTouchBegin())
            {
                mTouchSync = true;
                StartGameState(GAME_STATE_SHUFFLE_PLAY);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_PLAY:
        {
            // We're in active play, so track our time for scoring purposes
            mScoreTimer += dt;
            
            // Delay timer handles the GO! message.
            if (mDelayTimer > 0.0f)
            {
                UpdateTimer(mDelayTimer, dt);
            }
            
            // Check for holding, which enables the options menu.
            if (AnyTouchHold())
            {
                if (UpdateTimer(mOptionsTimer, dt))
                {
                    mTouchSync = true;
                    StartGameState(GAME_STATE_SHUFFLE_OPTIONS);
                }
            }
            else
            {
                mOptionsTimer = kOptionsTimerDuration;
            }
            
            if (mTouchSync)
            {
                if (AnyTouchBegin())
                {
                    mTouchSync = false;
                }
            }
            
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
            
            // Clue Stuff
            if (mHintFlowIndex == 0)
            {
                for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
                {
                    if (mCubeWrappers[i].IsEnabled())
                    {
                        if (mTouching[i] == TOUCH_STATE_BEGIN)
                        {
                            if (!mTouchSync)
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
                    }
                }
            }
            
            // If clues are not displayed, see if a hint should be turned on or off
            if (!clueDisplayed)
            {
                if (IsHinting())
                {
                    if (mHintTimer > 0.0f && mSwapState == SWAP_STATE_NONE)
                    {
                        if (UpdateTimer(mHintTimer, dt))
                        {
                            StopHint(false);
                        }
                        else if (AnyTouchBegin())
                        {
                            StopHint(true);
                        }
                    }
                }
                else
                {
                    if (mHintTimer > 0.0f && mSwapState == SWAP_STATE_NONE)
                    {
                        if (mHintFlowIndex == 0)
                        {
                            if (UpdateTimer(mHintTimer, dt))
                            {
                                ++mHintFlowIndex;
                                mHintTimer = kHintTimerRepeatDuration;
                            }
                        }
                        else if (mHintFlowIndex == 1)
                        {
                            if (UpdateTimer(mHintTimer, dt))
                            {
                                ++mHintFlowIndex;
                                mHintTimer = kHintTimerRepeatDuration;
                            }
                            else if (AnyTouchBegin())
                            {
                                mHintTimer = kHintTimerOnDuration;
                                mHintFlowIndex = 0;
                            }
                        }
                        else
                        {
                            if (UpdateTimer(mHintTimer, dt))
                            {
                                StartHint();
                            }
                        }
                    }
                }
            }
            break;
        }
        case GAME_STATE_SHUFFLE_OPTIONS:
        {   
            if (mTouchSync)
            {
                for (int i = 0; i < arraysize(mTouching); ++i)
                {
                    if (mTouching[i] == TOUCH_STATE_BEGIN)
                    {
                        mTouchSync = false;
                    }
                }
            }
            else
            {
                if (mTouchEndChoiceTimer > 0.0f)
                {
                    if (UpdateTimer(mTouchEndChoiceTimer, dt))
                    {
                        ASSERT(mTouchEndChoice >= 0 && mTouchEndChoice < 3);
                        if (mTouchEndChoice == 0)
                        {
                            mHintTimer = kHintTimerOnDuration;
                            mHintFlowIndex = 0;
                            for (unsigned int i = 0; i < arraysize(mClueOffTimers); ++i)
                            {
                                mClueOffTimers[i] = 0.0f;
                            }
                            PlaySound(SoundUnpause);
                            StartGameState(GAME_STATE_SHUFFLE_PLAY);
                            mDelayTimer = 0.0f; // Turn off GO! sprite
                        }
                        else if (mTouchEndChoice == 1)
                        {
                            ResetCubesToShuffleStart();
                            StartGameState(GAME_STATE_SHUFFLE_UNSHUFFLE_THE_FACES);
                        }
                        else if (mTouchEndChoice == 2)
                        {
                            StartGameState(GAME_STATE_MENU_MAIN);
                        }
                    }
                }
                else
                {
                    for (int i = 0; i < 3; ++i)
                    {
                        if (mTouching[i] == TOUCH_STATE_END)
                        {
                            mTouchEndChoiceTimer = kPushButtonDelay;
                            mTouchEndChoice = i;
                            break;
                        }
                    }
                }
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
            UpdateCutsceneSpriteJump(mCutsceneSpriteJump0, kShuffleCutsceneJumpChance, 1);
            
            if (UpdateTimer(mDelayTimer, dt) || AnyTouchBegin())
            {
                StartGameState(GAME_STATE_SHUFFLE_END_GAME_NAV);
            }
            else if (AnyTouchBegin())
            {
                mTouchSync = true;
                StartGameState(GAME_STATE_SHUFFLE_END_GAME_NAV);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_END_GAME_NAV:
        {
            if (mTouchSync)
            {
                for (int i = 0; i < arraysize(mTouching); ++i)
                {
                    if (mTouching[i] == TOUCH_STATE_BEGIN)
                    {
                        mTouchSync = false;
                    }
                }
            }
            else
            {
                if (mTouchEndChoiceTimer > 0.0f)
                {
                    if (UpdateTimer(mTouchEndChoiceTimer, dt))
                    {
                        ASSERT(mTouchEndChoice >= 1 && mTouchEndChoice < 3);
                        if (mTouchEndChoice == 1)
                        {
                            StartGameState(GAME_STATE_SHUFFLE_CHARACTER_SPLASH);
                        }
                        else if (mTouchEndChoice == 2)
                        {
                            StartGameState(GAME_STATE_MENU_MAIN);
                        }
                    }
                }
                else
                {
                    for (int i = 1; i < 3; ++i)
                    {
                        if (mTouching[i] == TOUCH_STATE_END)
                        {
                            mTouchEndChoiceTimer = kPushButtonDelay;
                            mTouchEndChoice = i;
                            break;
                        }
                    }
                }
            }
            break;
        }
        case GAME_STATE_STORY_BOOK_START:
        {
            if (UpdateTimer(mDelayTimer, dt) || AnyTouchBegin())
            {
                StartGameState(GAME_STATE_STORY_CHAPTER_START);
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
            UpdateCutsceneSpriteJump(mCutsceneSpriteJump0, kStoryCutsceneJumpChanceA, 1);
            UpdateCutsceneSpriteJump(mCutsceneSpriteJump1, kStoryCutsceneJumpChanceB, 1);
            
            if (UpdateTimer(mDelayTimer, dt))
            {
                if (++mStoryCutsceneIndex == GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetNumCutsceneLineStart())
                {
                    StartGameState(GAME_STATE_STORY_DISPLAY_START_STATE);
                }
                else
                {
                    PlaySound(SoundCutsceneChatter);
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
                if (GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetNumShuffles() > 0)
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
                    ShufflePieces(GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetNumBuddies());
                }
            }
            break;
        }
        case GAME_STATE_STORY_CLUE:
        {
            if (UpdateTimer(mDelayTimer, dt) || AnyTouchBegin())
            {
                StartGameState(GAME_STATE_STORY_PLAY);
            }
            break;
        }
        case GAME_STATE_STORY_PLAY:
        {
            // We're in active play, so track our time for scoring purposes
            mScoreTimer += dt;
            
            // Delay timer handles the GO! message.
            if (mDelayTimer > 0.0f)
            {
                UpdateTimer(mDelayTimer, dt);
            }
            
            // Check for holding, which enables the options menu.
            if (AnyTouchHold())
            {
                if (UpdateTimer(mOptionsTimer, dt))
                {
                    mTouchSync = true;
                    StartGameState(GAME_STATE_STORY_OPTIONS);
                }
            }
            else
            {
                mOptionsTimer = kOptionsTimerDuration;
            }
            
            if (mTouchSync)
            {
                if (AnyTouchBegin())
                {
                    mTouchSync = false;
                }
            }
            
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
            
            // Clue Stuff
            if (mHintFlowIndex == 0)
            {
                for (unsigned int i = 0; i < arraysize(mCubeWrappers); ++i)
                {
                    if (mCubeWrappers[i].IsEnabled())
                    {
                        if (mTouching[i] == TOUCH_STATE_BEGIN)
                        {
                            if (!mTouchSync)
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
                    }
                }
            }
            
            // If clues are not displayed, see if a hint should be turned on or off
            if (!clueDisplayed)
            {
                if (IsHinting())
                {
                    if (mHintTimer > 0.0f && mSwapState == SWAP_STATE_NONE)
                    {
                        if (UpdateTimer(mHintTimer, dt))
                        {
                            StopHint(false);
                        }
                        else if (AnyTouchBegin())
                        {
                            StopHint(true);
                        }
                    }
                }
                else
                {
                    if (mHintTimer > 0.0f && mSwapState == SWAP_STATE_NONE)
                    {
                        if (mHintFlowIndex == 0)
                        {
                            if (UpdateTimer(mHintTimer, dt))
                            {
                                ++mHintFlowIndex;
                                mHintTimer = kHintTimerRepeatDuration;
                            }
                        }
                        else if (mHintFlowIndex == 1)
                        {
                            if (UpdateTimer(mHintTimer, dt))
                            {
                                ++mHintFlowIndex;
                                mHintTimer = kHintTimerRepeatDuration;
                            }
                            else if (AnyTouchBegin())
                            {
                                mHintTimer = kHintTimerOnDuration;
                                mHintFlowIndex = 0;
                            }
                        }
                        else
                        {
                            if (UpdateTimer(mHintTimer, dt))
                            {
                                StartHint();
                            }
                        }
                    }
                }
            }
            break;
        }
        case GAME_STATE_STORY_OPTIONS:
        {   
            if (mTouchSync)
            {
                for (int i = 0; i < arraysize(mTouching); ++i)
                {
                    if (mTouching[i] == TOUCH_STATE_BEGIN)
                    {
                        mTouchSync = false;
                    }
                }
            }
            else
            {
                if (mTouchEndChoiceTimer > 0.0f)
                {
                    if (UpdateTimer(mTouchEndChoiceTimer, dt))
                    {
                        ASSERT(mTouchEndChoice >= 0 && mTouchEndChoice < 3);
                        if (mTouchEndChoice == 0)
                        {
                            mHintTimer = kHintTimerOnDuration;
                            mHintFlowIndex = 0;
                            for (unsigned int i = 0; i < arraysize(mClueOffTimers); ++i)
                            {
                                mClueOffTimers[i] = 0.0f;
                            }
                            PlaySound(SoundUnpause);
                            StartGameState(GAME_STATE_STORY_PLAY);
                            mDelayTimer = 0.0f; // Turn off GO! sprite
                        }
                        else if (mTouchEndChoice == 1)
                        {
                            StartGameState(GAME_STATE_STORY_CHAPTER_START);
                        }
                        else if (mTouchEndChoice == 2)
                        {
                            StartGameState(GAME_STATE_MENU_MAIN);
                        }
                    }
                }
                else
                {
                    for (int i = 0; i < 3; ++i)
                    {
                        if (mTouching[i] == TOUCH_STATE_END)
                        {
                            mTouchEndChoiceTimer = kPushButtonDelay;
                            mTouchEndChoice = i;
                            break;
                        }
                    }
                }
            }
            break;
        }
        case GAME_STATE_STORY_SOLVED:
        {
            if (mUiIndex == 0)
            {
                if (UpdateTimer(mDelayTimer, dt))
                {
                    ++mUiIndex;
                    mDelayTimer = kStateTimeDelayLong;
                }
            }
            else
            {
                if (UpdateTimer(mDelayTimer, dt) || AnyTouchBegin())
                {
                    StartGameState(GAME_STATE_STORY_CUTSCENE_END_1);
                }
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
            UpdateCutsceneSpriteJump(mCutsceneSpriteJump0, kStoryCutsceneJumpChanceA, 1);
            UpdateCutsceneSpriteJump(mCutsceneSpriteJump1, kStoryCutsceneJumpChanceB, 1);
            
            if (UpdateTimer(mDelayTimer, dt))
            {
                if (++mStoryCutsceneIndex == GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetNumCutsceneLineEnd())
                {
                    if (NextUnlockedBuddy() != -1)
                    {
                        StartGameState(GAME_STATE_UNLOCKED_1);
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
                if (NextUnlockedBuddy() != -1)
                {
                    StartGameState(GAME_STATE_UNLOCKED_1);
                }
                else
                {
                    mTouchSync = true;
                    StartGameState(GAME_STATE_STORY_CHAPTER_END);
                }
            }
            break;
        }
        case GAME_STATE_UNLOCKED_1:
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
                    if (AnyTouchBegin())
                    {
                        mDelayTimer = 0.0f;
                    }
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
                    StartGameState(GAME_STATE_UNLOCKED_2);
                }
            }
            break;
        }
        case GAME_STATE_UNLOCKED_2:
        {
            if (mDelayTimer > 0.0f)
            {
                if (mBackgroundScroll.y < 14)
                {
                    ++mBackgroundScroll.y;
                }
                else
                {
                    if (UpdateTimer(mDelayTimer, dt) || AnyTouchBegin())
                    {
                        int buddyId = NextUnlockedBuddy();
                        ASSERT(buddyId >= 0 && buddyId < NUM_BUDDIES);
                        
                        mStoryBuddyUnlockMask |= 1 << buddyId;
                        
                        if (NextUnlockedBuddy() != -1)
                        {
                            StartGameState(GAME_STATE_UNLOCKED_1);
                        }
                        else
                        {
                            if (mStoryPreGame)
                            {
                                mStoryPreGame = false;
                                StartGameState(GAME_STATE_MENU_STORY);
                            }
                            else
                            {
                                if (AnyTouchBegin())
                                {
                                    mTouchSync = true;
                                }
                                StartGameState(GAME_STATE_STORY_CHAPTER_END);
                            }
                        }
                    }
                    else
                    {
                        UpdateCutsceneSpriteJump(mCutsceneSpriteJump0, kStoryUnlockJumpChance, 1);
                    }
                }
            }
            break;
        }
        case GAME_STATE_STORY_CHAPTER_END:
        {
            if (mTouchSync)
            {
                for (int i = 0; i < arraysize(mTouching); ++i)
                {
                    if (mTouching[i] == TOUCH_STATE_BEGIN)
                    {
                        mTouchSync = false;
                    }
                }
            }
            else
            {
                if (mTouchEndChoiceTimer > 0.0f)
                {
                    if (UpdateTimer(mTouchEndChoiceTimer, dt))
                    {
                        ASSERT(mTouchEndChoice >= 0 && mTouchEndChoice < 3);
                        if (mTouchEndChoice == 0)
                        {
                            if (++mStoryPuzzleIndex == GetBook(mStoryBookIndex).mNumPuzzles)
                            {
                                ++mStoryBookIndex;
                                mStoryPuzzleIndex = 0;
                                
                                if (mStoryBookIndex > mSaveDataStoryBookProgress)
                                {
                                    mSaveDataStoryPuzzleProgress = mStoryPuzzleIndex;
                                    mSaveDataStoryBookProgress = mStoryBookIndex;
                                    SaveData();
                                }
                                
                                if (mStoryBookIndex == GetNumBooks())
                                {
                                    StartGameState(GAME_STATE_STORY_GAME_END_CONGRATS);
                                }
                                else
                                {
                                    StartGameState(GAME_STATE_STORY_BOOK_START);
                                }
                            }
                            else
                            {
                                if (mStoryPuzzleIndex > mSaveDataStoryPuzzleProgress)
                                {
                                    mSaveDataStoryPuzzleProgress = mStoryPuzzleIndex;
                                    SaveData();
                                }
                                StartGameState(GAME_STATE_STORY_CHAPTER_START);
                            }
                        }
                        else if (mTouchEndChoice == 1)
                        {
                            StartGameState(GAME_STATE_STORY_CHAPTER_START);
                        }
                        else if (mTouchEndChoice == 2)
                        {
                            StartGameState(GAME_STATE_MENU_MAIN);
                        }
                    }
                }
                else
                {
                    for (int i = 0; i < 3; ++i)
                    {
                        if (mTouching[i] == TOUCH_STATE_END)
                        {
                            mTouchEndChoiceTimer = kPushButtonDelay;
                            mTouchEndChoice = i;
                            break;
                        }
                    }
                }
            }
            break;
        }
        case GAME_STATE_STORY_GAME_END_CONGRATS:
        {
            if (UpdateTimer(mDelayTimer, dt) || AnyTouchBegin())
            {
                if (GetNumBuddiesLeftToUnlock() > 0)
                {
                    StartGameState(GAME_STATE_STORY_GAME_END_MORE);
                }
                else
                {
                    StartGameState(GAME_STATE_MENU_MAIN);
                }
            }
            break;
        }
        case GAME_STATE_STORY_GAME_END_MORE:
        {
            if (UpdateTimer(mDelayTimer, dt) || AnyTouchBegin())
            {
                StartGameState(GAME_STATE_MENU_MAIN);
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
        case GAME_STATE_TITLE:
        {
            cubeWrapper.DrawBackground(UiTitleScreen);
            break;
        }
        case GAME_STATE_FREEPLAY_TITLE:
        {
            cubeWrapper.DrawBackground(FreePlayTitle);
            break;
        }
        case GAME_STATE_FREEPLAY_DESCRIPTION:
        {
            cubeWrapper.DrawBackground(FreePlayDescription);
            break;
        }
        case GAME_STATE_FREEPLAY_PLAY:
        {
            cubeWrapper.DrawBuddy();
            break;
        }
        case GAME_STATE_FREEPLAY_OPTIONS:
        {
            if (cubeWrapper.GetId() == 0 || cubeWrapper.GetId() > 2)
            {
                DrawBackgroundWithTouchBump(cubeWrapper, UiResume);
            }
            else if (cubeWrapper.GetId() == 1)
            {
                DrawBackgroundWithTouchBump(cubeWrapper, UiRestart);
            }
            else if (cubeWrapper.GetId() == 2)
            {
                DrawBackgroundWithTouchBump(cubeWrapper, UiEndGameNavExit);
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
            cubeWrapper.DrawBackground(ShuffleMemorizeFaces);
            break;
        }
        case GAME_STATE_SHUFFLE_CHARACTER_SPLASH:
        {
            cubeWrapper.DrawBuddy();
            break;
        }
        case GAME_STATE_SHUFFLE_SHAKE_TO_SHUFFLE:
        {
            ASSERT(cubeWrapper.GetBuddyId() < arraysize(kBuddiesFull));
            cubeWrapper.DrawBackground(*kBuddiesFull[cubeWrapper.GetBuddyId()]);
            
            if (mUiIndex == 0 && !mUiIndexSync[cubeWrapper.GetId()])
            {
                cubeWrapper.DrawUiAsset(Vec2(0, 0), ShuffleTouchToSwap);
            }
            else if (mUiIndex == 1 && !mUiIndexSync[cubeWrapper.GetId()])
            {
                cubeWrapper.DrawUiAsset(Vec2(0, 0), ShuffleShakeToShuffle);
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
            if (cubeWrapper.GetId() == 0)
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
            if (mClueOffTimers[cubeWrapper.GetId()] > 0.0f)
            {
                // Clue
                ASSERT(cubeWrapper.GetBuddyId() < arraysize(kBuddiesFull));
                cubeWrapper.DrawBackground(*kBuddiesFull[cubeWrapper.GetBuddyId()]);
            }
            else if (cubeWrapper.GetId() == 0 && mHintFlowIndex == 1)
            {
                // Hint 1
                cubeWrapper.DrawBackground(ShuffleNeighbor);
            }
            else if (mFaceCompleteTimers[cubeWrapper.GetId()] > 0.0f)
            {
                ASSERT(cubeWrapper.GetBuddyId() < arraysize(kBuddiesFull));
                cubeWrapper.DrawBackground(*kBuddiesFull[cubeWrapper.GetBuddyId()]);
                cubeWrapper.DrawUiAsset(Vec2(0, 0), UiBannerFaceCompleteOrange);
            }
            else
            {
                cubeWrapper.DrawBuddy();
                
                if (mDelayTimer > 0.0f)
                {
                    cubeWrapper.DrawSprite(0, Vec2(48, 48), UiGoOrange);
                }
            }
            break;
        }
        case GAME_STATE_SHUFFLE_OPTIONS:
        {
            if (cubeWrapper.GetId() == 0 || cubeWrapper.GetId() >  2)
            {
                DrawBackgroundWithTouchBump(cubeWrapper, UiResume);
            }
            else if (cubeWrapper.GetId() == 1)
            {
                DrawBackgroundWithTouchBump(cubeWrapper, UiRestart);
            }
            else if (cubeWrapper.GetId() == 2)
            {
                DrawBackgroundWithTouchBump(cubeWrapper, UiEndGameNavExit);
            }
            break;
        }
        case GAME_STATE_SHUFFLE_SOLVED:
        {
            if (mFaceCompleteTimers[cubeWrapper.GetId()] > 0.0f)
            {
                ASSERT(cubeWrapper.GetBuddyId() < arraysize(kBuddiesFull));
                cubeWrapper.DrawBackground(*kBuddiesFull[cubeWrapper.GetBuddyId()]);
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
            DrawShuffleCutscene(cubeWrapper, Vec2(0, 0), cubeWrapper.GetBuddyId(), mCutsceneSpriteJump0);
            break;
        }
        case GAME_STATE_SHUFFLE_END_GAME_NAV:
        {
            if (cubeWrapper.GetId() == 0 || cubeWrapper.GetId() > 2)
            {
                DrawShuffleScore(*this, cubeWrapper, mScoreTimer, mScorePlace);
            }
            else if (cubeWrapper.GetId() == 1)
            {
                DrawBackgroundWithTouchBump(cubeWrapper, ShuffleEndGameNavReplay);
            }
            else if (cubeWrapper.GetId() == 2)
            {
                DrawBackgroundWithTouchBump(cubeWrapper, UiEndGameNavExit);
            }
            break;
        }
        case GAME_STATE_STORY_BOOK_START:
        {
            DrawStoryBookTitle(cubeWrapper, mStoryBookIndex, mStoryPuzzleIndex);
            break;
        }
        case GAME_STATE_STORY_CHAPTER_START:
        {
            DrawStoryChapterTitle(cubeWrapper, mStoryBookIndex, mStoryPuzzleIndex);
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_START:
        {
            if (cubeWrapper.GetId() == 0)
            {
                DrawStoryCutscene(
                    cubeWrapper,
                    GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetCutsceneEnvironment(),
                    GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetCutsceneLineStart(mStoryCutsceneIndex),
                    GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetCutsceneBuddyStart(0),
                    GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetCutsceneBuddyStart(1),
                    mCutsceneSpriteJump0,
                    mCutsceneSpriteJump1);
            }
            else
            {
                DrawStoryChapterTitle(cubeWrapper, mStoryBookIndex, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_DISPLAY_START_STATE:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetNumBuddies())
            {
                cubeWrapper.DrawBuddy();
            }
            else
            {
                DrawStoryChapterTitle(cubeWrapper, mStoryBookIndex, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_SCRAMBLING:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetNumBuddies())
            {
                cubeWrapper.DrawBuddy();
            }
            else
            {
                DrawStoryChapterTitle(cubeWrapper, mStoryBookIndex, mStoryPuzzleIndex);
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
                    GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetClue());
            }
            else if (cubeWrapper.GetId() < GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetNumBuddies())
            {
                cubeWrapper.DrawBuddy();
            }
            else
            {
                DrawStoryChapterTitle(cubeWrapper, mStoryBookIndex, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_PLAY:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetNumBuddies())
            {
                if (mClueOffTimers[cubeWrapper.GetId()] > 0.0f)
                {
                    // Clue
                    DrawStoryClue(
                        cubeWrapper,
                        mStoryPuzzleIndex,
                        UiClueBlank,
                        GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetClue());
                }
                else if (cubeWrapper.GetId() == 0 && mHintFlowIndex == 1)
                {
                    // Hint 1
                    cubeWrapper.DrawBackground(StoryNeighbor);
                }
                else if (mFaceCompleteTimers[cubeWrapper.GetId()] > 0.0f)
                {
                    DrawStoryFaceComplete(cubeWrapper);
                }
                else
                {
                    cubeWrapper.DrawBuddy();
                    
                    if (mDelayTimer > 0.0f)
                    {
                        cubeWrapper.DrawSprite(0, Vec2(48, 48), UiGoOrange);
                    }
                }
            }
            else
            {
                DrawStoryChapterTitle(cubeWrapper, mStoryBookIndex, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_OPTIONS:
        {
            if (cubeWrapper.GetId() == 0 || cubeWrapper.GetId() >  2)
            {
                DrawBackgroundWithTouchBump(cubeWrapper, UiResume);
            }
            else if (cubeWrapper.GetId() == 1)
            {
                DrawBackgroundWithTouchBump(cubeWrapper, UiRestart);
            }
            else if (cubeWrapper.GetId() == 2)
            {
                DrawBackgroundWithTouchBump(cubeWrapper, UiEndGameNavExit);
            }
            break;
        }
        case GAME_STATE_STORY_SOLVED:
        {
            if (cubeWrapper.GetId() < GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetNumBuddies())
            {
                if (mUiIndex > 0 && mFaceCompleteTimers[cubeWrapper.GetId()] > 0.0f)
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
                DrawStoryChapterTitle(cubeWrapper, mStoryBookIndex, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_END_1:
        {
            DrawStoryProgress(cubeWrapper, mStoryBookIndex, mStoryPuzzleIndex);
            break;
        }
        case GAME_STATE_STORY_CUTSCENE_END_2:
        {
            if (cubeWrapper.GetId() == 0)
            {
                DrawStoryCutscene(
                    cubeWrapper,
                    GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetCutsceneEnvironment(),
                    GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetCutsceneLineEnd(mStoryCutsceneIndex),
                    GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetCutsceneBuddyEnd(0),
                    GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetCutsceneBuddyEnd(1),
                    mCutsceneSpriteJump0,
                    mCutsceneSpriteJump1);
            }
            else
            {
                DrawStoryProgress(cubeWrapper, mStoryBookIndex, mStoryPuzzleIndex);
            }
            break;
        }
        case GAME_STATE_UNLOCKED_1:
        {
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
        case GAME_STATE_UNLOCKED_2:
        {
            cubeWrapper.DrawBackground(UiCongratulations);
            
            if (mBackgroundScroll.y > 0 && mBackgroundScroll.y < 20)
            {
                int buddyId = NextUnlockedBuddy();
                ASSERT(buddyId >= 0 && buddyId < NUM_BUDDIES);
                
                ASSERT(buddyId < arraysize(kBuddyRibbons));
                const AssetImage &ribbon = *kBuddyRibbons[buddyId];
                
                int y = mBackgroundScroll.y - ribbon.height;
                int assetOffset = 0;
                int assetHeight = ribbon.height;
                
                if (mBackgroundScroll.y < int(ribbon.height))
                {
                    y = 0;
                    assetOffset = ribbon.height - mBackgroundScroll.y;
                    assetHeight = mBackgroundScroll.y;
                }
                else if (mBackgroundScroll.y > kMaxTilesY)
                {
                    assetOffset = 0;
                    assetHeight = ribbon.height - (mBackgroundScroll.y - kMaxTilesY);
                }
                
                DrawUnlocked3Sprite(cubeWrapper, BuddyId(buddyId), mBackgroundScroll, mCutsceneSpriteJump0);
                
                ASSERT(assetOffset >= 0 && assetOffset <  int(ribbon.height));
                ASSERT(assetHeight >  0 && assetHeight <= int(ribbon.height));
                cubeWrapper.DrawUiAssetPartial(
                    Vec2(0, y),
                    Vec2(0, assetOffset),
                    Vec2(kMaxTilesX, assetHeight),
                    ribbon);
            }
            break;
        }
        case GAME_STATE_STORY_CHAPTER_END:
        {
            if (cubeWrapper.GetId() == 0)
            {
                if ((mStoryPuzzleIndex + 1) == GetBook(mStoryBookIndex).mNumPuzzles)
                {
                    DrawBackgroundWithTouchBump(cubeWrapper, StoryBookStartNext);
                    
                    ASSERT((mStoryBookIndex + 1) < GetNumBooks());
                    BuddyId buddyId = GetBook(mStoryBookIndex + 1).mUnlockBuddyId;
                    
                    ASSERT(buddyId < arraysize(kBuddySpritesFront));
                    cubeWrapper.DrawSprite(0, Vec2(32, 14), *kBuddySpritesFront[buddyId]);
                }
                else
                {
                    DrawStoryChapterNext(cubeWrapper, mStoryBookIndex, mStoryPuzzleIndex);
                }
            }
            else if (cubeWrapper.GetId() == 1)
            {
                DrawStoryChapterRetry(cubeWrapper, mStoryPuzzleIndex);
            }
            else if (cubeWrapper.GetId() == 2)
            {
                DrawBackgroundWithTouchBump(cubeWrapper, UiEndGameNavExit);
            }
            break;
        }
        case GAME_STATE_STORY_GAME_END_CONGRATS:
        {
            cubeWrapper.DrawBackground(UiCongratulations);
            cubeWrapper.DrawUiAsset(Vec2(0, 7), StoryRibbonComplete);
            break;
        }
        case GAME_STATE_STORY_GAME_END_MORE:
        {
            cubeWrapper.DrawBackground(StoryCongratulationsUnlock);
            
            int numUnlockLeft = GetNumBuddiesLeftToUnlock();
            int xSpan = numUnlockLeft * 2 + (numUnlockLeft - 1) * 1;
            int xBase = (VidMode::LCD_width / VidMode::TILE / 2) - (xSpan / 2);
            
            // No invisible buddies!
            int iFace = 0;
            for (int iBuddy = 0; iBuddy < (NUM_BUDDIES - 1); ++iBuddy)
            {
                if ((mSaveDataBuddyUnlockMask & (1 << iBuddy)) == 0)
                {
                    int x = xBase + iFace * 3;
                    
                    ASSERT(iBuddy < arraysize(kBuddiesSmall));
                    cubeWrapper.DrawUiAsset(Vec2(x, 5), *kBuddiesSmall[iBuddy]);
                    
                    ++iFace;
                }
            }
            
            if (numUnlockLeft % 2 == 0)
            {
                cubeWrapper.ScrollUi(Vec2(-VidMode::TILE / 2, 0U));
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
#if 0
    LOG(("SaveData = (%u, %u, %.2ff, %.2ff, %.2ff)\n",
        mSaveDataStoryBookProgress, mSaveDataStoryPuzzleProgress, mSaveDataBestTimes[0], mSaveDataBestTimes[1], mSaveDataBestTimes[1]));
    
    FILE *saveDataFile = std::fopen("SaveData.bin", "wb");
    ASSERT(saveDataFile != NULL);
    
    int numWritten0 =
        std::fwrite(&mSaveDataStoryBookProgress, sizeof(mSaveDataStoryBookProgress), 1, saveDataFile);
    ASSERT(numWritten0 == 1);
    
    int numWritten1 =
        std::fwrite(&mSaveDataStoryPuzzleProgress, sizeof(mSaveDataStoryPuzzleProgress), 1, saveDataFile);
    ASSERT(numWritten1 == 1);
    
    int numWritten2 =
        std::fwrite(mSaveDataBestTimes, sizeof(mSaveDataBestTimes[0]), arraysize(mSaveDataBestTimes), saveDataFile);
    ASSERT(numWritten2 == arraysize(mSaveDataBestTimes));
    
    int success = std::fclose(saveDataFile);
    ASSERT(success == 0);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::LoadData()
{
#if 0
    if (FILE *saveDataFile = fopen("SaveData.bin", "rb"))
    {
        int success0 = std::fseek(saveDataFile, 0L, SEEK_END);
        ASSERT(success0 == 0);
        
        std::size_t sizeFile = std::ftell(saveDataFile);
        std::sizt_t sizeData = sizeof(mSaveDataStoryBookProgress) + sizeof(mSaveDataStoryPuzzleProgress) + sizeof(mSaveDataBestTimes);
        
        if (sizeFile != sizeData)
        {
            LOG(("SaveData.bin is wrong size. Re-saving...\n"));
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
            
            LOG(("SaveData = (%u, %u, %.2ff, %.2ff, %.2ff)\n",
                mSaveDataStoryBookProgress, mSaveDataStoryPuzzleProgress, mSaveDataBestTimes[0], mSaveDataBestTimes[1], mSaveDataBestTimes[1]));
        }
    }
#else
    mSaveDataStoryBookProgress = 0;
    mSaveDataStoryPuzzleProgress = 0;
    ASSERT(GetNumBooks() > 0);
    ASSERT(GetBook(0).mUnlockBuddyId >= 0 && GetBook(0).mUnlockBuddyId < NUM_BUDDIES);
    mSaveDataBuddyUnlockMask = 1 << GetBook(0).mUnlockBuddyId;
    mSaveDataBestTimes[0] = 0.0f;
    mSaveDataBestTimes[1] = 0.0f;
    mSaveDataBestTimes[2] = 0.0f;
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
    
    PlaySound(SoundPieceSlide);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////              

void App::OnSwapExchange()
{
    mSwapState = SWAP_STATE_IN;
    mSwapAnimationSlideTimer = kSwapAnimationSlide;
    
    mSwapAnimationRotateTimer = kSwapAnimationSlide / NUM_SIDES;
    mSwapAnimationRotateTimer *= 2.0f; // Double the delay for first rotation, so we can see it
    
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
            PlaySound(SoundFaceComplete);
        }
        else
        {
            PlaySound(SoundPieceLand);
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
        
        PlaySound(SoundPieceLand);
        
        if (done)
        {
            PlaySound(SoundShuffleEnd);
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
        
        if (AllSolved(*this))
        {
            PlaySound(SoundFaceCompleteAll);
        }
        else if (swap0Solved || swap1Solved)
        {
            PlaySound(SoundFaceComplete);
        }
        else
        {
            PlaySound(SoundPieceLand);
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
        
        if (GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetNumShuffles() > 0)
        {
            done = done || mShuffleMoveCounter == GetPuzzle(mStoryBookIndex, mStoryPuzzleIndex).GetNumShuffles();
        }
        
        PlaySound(SoundPieceLand);
        
        if (done)
        {
            PlaySound(SoundShuffleEnd);
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
        
        if (AllSolved(*this))
        {
            PlaySound(SoundFaceComplete);
        }
        else if (swap0Solved || swap1Solved)
        {
            PlaySound(SoundFaceCompleteAll);
        }
        else
        {
            PlaySound(SoundPieceLand);
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
    
    // Check all sides of all cubes against each other, looking for a double swap for pieces that
    // are both out of place.
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
                            
                            if (piece0.GetAttribute() != Piece::ATTR_FIXED &&
                                piece1.GetAttribute() != Piece::ATTR_FIXED)
                            {
                                const Piece &pieceSolution0 =
                                    mCubeWrappers[iCube0].GetPieceSolution(iSide0);
                                const Piece &pieceSolution1 =
                                    mCubeWrappers[iCube1].GetPieceSolution(iSide1);
                                
                                if (!piece0.Compare(pieceSolution0) &&
                                    !piece1.Compare(pieceSolution1))
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

void App::UpdateTouch()
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

void App::DrawBackgroundWithTouchBump(CubeWrapper &cubeWrapper, const AssetImage &background)
{
    bool holding =
        !mTouchSync &&
        mTouchEndChoiceTimer == 0.0f &&
        arraysize(mTouching) > cubeWrapper.GetId() &&
        mTouching[cubeWrapper.GetId()] == TOUCH_STATE_HOLD;
    
    cubeWrapper.DrawBackgroundPartial(
        Vec2(0, 0),
        Vec2(1, holding ? 2 : 1),
        Vec2(16, 16),                      
        background);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::DrawStoryChapterNext(CubeWrapper &cubeWrapper, unsigned int bookIndex, unsigned int puzzleIndex)
{
    DrawBackgroundWithTouchBump(cubeWrapper, StoryChapterNext);
    
    unsigned int nextPuzzleIndex = ++puzzleIndex % GetBook(bookIndex).mNumPuzzles;

    String<16> buffer;
    buffer << "Chapter " << (nextPuzzleIndex + 1);
    int x = (kMaxTilesX / 2) - (buffer.size() / 2);
    cubeWrapper.DrawUiText(Vec2(x, 9), UiFontOrange, buffer.c_str());
    
    Int2 scroll;
    scroll.x = 0;
    scroll.y = VidMode::TILE / 2;
    if (buffer.size() % 2 != 0)
    {
        scroll.x = VidMode::TILE / 2;
    }
    cubeWrapper.ScrollUi(scroll);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::DrawStoryChapterRetry(CubeWrapper &cubeWrapper, unsigned int puzzleIndex)
{
    DrawBackgroundWithTouchBump(cubeWrapper, StoryChapterRetry);
    
    String<16> buffer;
    buffer << "Chapter " << (puzzleIndex + 1);
    int x = (kMaxTilesX / 2) - (buffer.size() / 2);
    
    cubeWrapper.DrawUiText(Vec2(x, 9), UiFontOrange, buffer.c_str());
    
    Int2 scroll;
    scroll.x = 0;
    scroll.y = VidMode::TILE / 2;
    if (buffer.size() % 2 != 0)
    {
        scroll.x = VidMode::TILE / 2;
    }
    cubeWrapper.ScrollUi(scroll);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

int App::NextUnlockedBuddy() const
{
    for (int i = 0; i < GetNumBooks(); ++i)
    {
        unsigned int buddyMask = 1 << GetBook(i).mUnlockBuddyId;
        
        bool inSaveData = (mSaveDataBuddyUnlockMask & buddyMask) != 0;
        bool inRuntimeData = (mStoryBuddyUnlockMask & buddyMask) != 0;
        
        if (inSaveData && !inRuntimeData)
        {
            return GetBook(i).mUnlockBuddyId;
        }
    }
    
    return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

int App::GetNumBuddiesLeftToUnlock() const
{
    // No invisible buddies!
    
    int numLeftToUnlock = 0;
    for (int i = 0; i < (NUM_BUDDIES - 1); ++i)
    {
        if ((mSaveDataBuddyUnlockMask & (1 << i)) == 0)
        {
            ++numLeftToUnlock;
        }
    }
    return numLeftToUnlock;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void App::UpdateCutsceneSpriteJump(bool &cutsceneSpriteJump, int upChance, int downChance)
{
    if (!cutsceneSpriteJump)
    {
        if (mCutsceneSpriteJumpRandom.randrange(upChance) == 0)
        {
            cutsceneSpriteJump = true;
        }
    }
    else
    {
        if (mCutsceneSpriteJumpRandom.randrange(downChance) == 0)
        {
            cutsceneSpriteJump = false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}
