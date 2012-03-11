#include "Anim.h"
#include "GameStateMachine.h"
#include "assets.gen.h"

enum Layer
{
    Layer_BG0,
    Layer_Sprite,
    Layer_BG1,
};

struct AnimObjData
{
    const AssetImage *mAsset;
    const AssetImage *mAltAsset;
    const PinnedAssetImage *mSpriteAsset;
    Layer mLayer : 2;
    uint16_t mInvisibleFrames; // bitmask
    unsigned char mNumFrames;
    const Vec2 *mPositions;
};

struct AnimData
{
    float mDuration;
    bool mLoop;
    unsigned char mNumObjs;
    const AnimObjData *mObjs;
};

// FIXME write a tool to hide all this array/struct nesting ugliness
// FIXME reuse stuff with indexing
const static Vec2 positions[] =
{
    Vec2(2, 2),
    Vec2(8, 2),
    Vec2(7, 2),
    Vec2(6, 2),
    Vec2(5, 2),
    Vec2(4, 2),
    Vec2(3, 2),
    Vec2(2, 2),
    Vec2(2, 2),
    Vec2(2, 2),
    Vec2(2, 2), // [10]
    Vec2(3, 2),
    Vec2(4, 2),
    Vec2(5, 2),
    Vec2(6, 2),
    Vec2(7, 2),
    Vec2(8, 2),
    Vec2(8, 2),
    Vec2(8, 2),
    Vec2(8, 2),
    Vec2(7, 2), // [20]
    Vec2(6, 2),
    Vec2(5, 2),
    Vec2(4, 2),
    Vec2(3, 2),
    Vec2(2, 2),
    Vec2(3, 3), // [26]
    Vec2(56, 2), // [27]
    Vec2(24, 2), // [28]
    Vec2(56, 2), // [29]
    Vec2(54, 2), // [30]
    Vec2(52, 2), // [31]
    Vec2(48, 2), // [32]
    Vec2(40, 2), // [33]
    Vec2(32, 2), // [34]
    Vec2(28, 2), // [35]
    Vec2(24, 2), // [36]
    Vec2(2, 2), // [37]
    Vec2(6, 2), // [38]
    Vec2(8, 2), // [39]
    Vec2(12, 2), // [40]
    Vec2(2, 11), // [41]
    Vec2(6, 11), // [42]
    Vec2(8, 11), // [43]
    Vec2(12, 11), // [44]
    Vec2(53, 2), // [45]
    Vec2(59, 2), // [27]
    Vec2(52, 2), // [27]
    Vec2(60, 2), // [27]
    Vec2(54, 2), // [27]
    Vec2(58, 2), // [27]
    Vec2(54, 2), // [27]
    Vec2(58, 2), // [27]
};

const static AnimObjData animObjData[] =
{    
    {&Tile2, &Tile2, 0, Layer_BG0, 0x0, 1, &positions[0]},// AnimIndex_Tile2Idle
    {&Tile2, &Tile2, 0, Layer_BG0, 0x0, 1, &positions[1]},
    {&Tile2, &Tile2, 0, Layer_BG0, 0x0, 10, &positions[7]}, // AnimIndex_Tile2SlideL
    {&Tile2, &Tile2, 0, Layer_BG0, 0x0, 7, &positions[2]},    
    {&Tile2, &Tile2, 0, Layer_BG0, 0x0, 7, &positions[10]}, // AnimIndex_Tile2SlideR
    {&Tile2, &Tile2, 0, Layer_BG0, 0x0, 10, &positions[16]},    
    {&Tile2Glow, &Tile2, 0, Layer_BG0, 0x0, 1, &positions[0]}, // AnimIndex_Tile2OldWord
    {&Tile2Glow, &Tile2Glow, 0, Layer_BG0, 0x0, 1, &positions[1]},
    {&LevelComplete , &LevelComplete, 0, Layer_BG1, 0x0, 1, &positions[26]}, // CityProgression
    { 0, 0, &HintSprite, Layer_Sprite, 0x0, 1, &positions[27]}, // HintIdle
    { 0, 0, &HintSprite, Layer_Sprite, 0x0, 1, &positions[28]}, // HintLocked
    {&Tile2, &Tile2, 0, Layer_BG0, 0x0, 10, &positions[7]}, // AnimType_SlideLHint
    {&Tile2, &Tile2, 0, Layer_BG0, 0x0, 7, &positions[2]},
    { 0, 0, &HintSprite, Layer_Sprite, 0x0, 8, &positions[29]},
    {&Tile2, &Tile2, 0, Layer_BG0, 0x0, 7, &positions[10]}, // AnimType_SlideRHint
    {&Tile2, &Tile2, 0, Layer_BG0, 0x0, 10, &positions[16]},
    { 0, 0, &HintSprite, Layer_Sprite, 0x0, 8, &positions[29]}, // FIXME keyframe
    {&Tile2, &Tile2, 0, Layer_BG0, 0x0, 1, &positions[0]},// AnimIndex_Tile2Idle
    {&Tile2, &Tile2, 0, Layer_BG0, 0x0, 1, &positions[1]},
    {&BorderLockedUL, &BorderLockedUL, 0, Layer_BG1, 0x0, 1, &positions[37]},
    {&BorderLockedUR, &BorderLockedUR, 0, Layer_BG1, 0x0, 1, &positions[38]},
    {&BorderLockedUL, &BorderLockedUL, 0, Layer_BG1, 0x0, 1, &positions[39]},
    {&BorderLockedUR, &BorderLockedUR, 0, Layer_BG1, 0x0, 1, &positions[40]},
    {&BorderLockedBL, &BorderLockedBL, 0, Layer_BG1, 0x0, 1, &positions[41]},
    {&BorderLockedBR, &BorderLockedBR, 0, Layer_BG1, 0x0, 1, &positions[42]},
    {&BorderLockedBL, &BorderLockedBL, 0, Layer_BG1, 0x0, 1, &positions[43]},
    {&BorderLockedBR, &BorderLockedBR, 0, Layer_BG1, 0x0, 1, &positions[44]},
    {&Tile2Glow, &Tile2Glow, 0, Layer_BG0, 0x0, 1, &positions[0]},// AnimIndex_Tile2Idle
    {&Tile2Glow, &Tile2Glow, 0, Layer_BG0, 0x0, 1, &positions[1]},
    {&BorderLockedUL, &BorderLockedUL, 0, Layer_BG1, 0x0, 1, &positions[37]},
    {&BorderLockedUR, &BorderLockedUR, 0, Layer_BG1, 0x0, 1, &positions[38]},
    {&BorderLockedUL, &BorderLockedUL, 0, Layer_BG1, 0x0, 1, &positions[39]},
    {&BorderLockedUR, &BorderLockedUR, 0, Layer_BG1, 0x0, 1, &positions[40]},
    {&BorderLockedBL, &BorderLockedBL, 0, Layer_BG1, 0x0, 1, &positions[41]},
    {&BorderLockedBR, &BorderLockedBR, 0, Layer_BG1, 0x0, 1, &positions[42]},
    {&BorderLockedBL, &BorderLockedBL, 0, Layer_BG1, 0x0, 1, &positions[43]},
    {&BorderLockedBR, &BorderLockedBR, 0, Layer_BG1, 0x0, 1, &positions[44]},
    { 0, 0, &HintSprite, Layer_Sprite, 0x0, 8, &positions[45]}, // HintShake
};

const static AnimData animData[] =
{
    //AnimIndex_Tile1Idle,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile1SlideL,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile1SlideR,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile1OldWord,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile1NewWord,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile1EndofRoundScored,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile1ShuffleScored,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile1CityProgression
    { 1.f, true, 1, &animObjData[0]},
    //AnimType_HintAppear,
    { 1.f, true, 1, &animObjData[0]},
    //AnimType_HintIdle,
    { 1.f, true, 1, &animObjData[0]},
    //AnimType_HintShake,
    { 1.f, true, 1, &animObjData[0]},
    //AnimType_HintDisappear,
    { 1.f, true, 1, &animObjData[0]},
    //AnimType_SlideLHint,
    { 1.f, true, 1, &animObjData[0]},
    //AnimType_SlideRHint,
    { 1.f, true, 1, &animObjData[0]},
    //AnimType_LockHint,
    { 1.f, true, 1, &animObjData[0]},
    //AnimType_LockedHintNotWord,
    { 1.f, true, 1, &animObjData[0]},
    //AnimType_LockedHintOldWord,
    { 1.f, true, 1, &animObjData[0]},

    // AnimIndex_Tile2Idle
    { 1.f, true, 2, &animObjData[0]},
    // AnimIndex_Tile2SlideL
    { 0.5f, false, 2, &animObjData[2]},
    // AnimIndex_Tile2SlideR
    { 0.5f, false, 2, &animObjData[4]},
    // AnimIndex_Tile2OldWord
    { 1.f, true, 2, &animObjData[6]},
    //AnimIndex_Tile2NewWord,
    { 1.5f, true, 2, &animObjData[6]},
    //AnimIndex_Tile2EndofRoundScored,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile2ShuffleScored,
    { 0.5f, true, 2, &animObjData[0]},
    //AnimIndex_Tile2CityProgression
    { 1.f, true, 1, &animObjData[8]},
    //AnimType_HintAppear,
    { 1.f, true, 1, &animObjData[0]},
    //AnimType_HintIdle,
    { 3.f, false, 1, &animObjData[9]},
    //AnimType_HintShake,
    { 0.3f, false, 1, &animObjData[37]},
    //AnimType_HintDisappear,
    { 1.f, true, 1, &animObjData[0]},
    // AnimIndex_HIntSlideL
    { 0.5f, false, 3, &animObjData[11]},
    // AnimIndex_HIntSlideR
    { 0.5f, false, 3, &animObjData[14]},
    //AnimType_LockHint,
    { 1.f, true, 2, &animObjData[0]},
    //AnimType_LockedHintNotWord,
    { 1.f, true, 10, &animObjData[17]},
    //AnimType_LockedHintOldWord,
    { 1.f, true, 10, &animObjData[27]},

    //AnimIndex_Tile3Idle,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile3SlideL,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile3SlideR,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile3OldWord,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile3NewWord,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile3EndofRoundScored,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile3ShuffleScored,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile3CityProgression
    { 1.f, true, 1, &animObjData[8]},
    //AnimType_HintAppear,
    { 1.f, true, 1, &animObjData[0]},
    //AnimType_HintIdle,
    { 1.f, true, 1, &animObjData[9]},
    //AnimType_HintShake,
    { 1.f, true, 1, &animObjData[0]},
    //AnimType_HintDisappear,
    { 1.f, true, 1, &animObjData[0]},
    //AnimType_SlideLHint,
    { 1.f, true, 1, &animObjData[0]},
    //AnimType_SlideRHint,
    { 1.f, true, 1, &animObjData[0]},
    //AnimType_LockHint,
    { 1.f, true, 1, &animObjData[0]},
    //AnimType_LockedHintNotWord,
    { 1.f, true, 10, &animObjData[17]},
    //AnimType_LockedHintOldWord,
    { 1.f, true, 10, &animObjData[27]},
};

bool animPaint(AnimType animT,
               VidMode_BG0_SPR_BG1 &vid,
               BG1Helper *bg1,
               float animTime,
               const AnimParams *params)
{
    const static AssetImage* fonts[] =
    {
        &Font1Letter, &Font2Letter, &Font3Letter,
    };
    const AssetImage& font = *fonts[GameStateMachine::getCurrentMaxLettersPerCube() - 1];
    if (animT == AnimType_None)
    {
        return false;
    }
    unsigned anim = (animT + NumAnimTypes * (GameStateMachine::getCurrentMaxLettersPerCube() - 1));
    STATIC_ASSERT(NumAnimTypes * MAX_LETTERS_PER_CUBE == arraysize(animData));
    //STATIC_ASSERT(arraysize(animData) == NumAnimIndexes);
    const AnimData &data = animData[anim];

    float animPct =
            data.mLoop ?
                fmodf(animTime, data.mDuration)/data.mDuration :
                MIN(1.f, animTime/data.mDuration);
    const int MAX_ROWS = 16, MAX_COLS = 16;
    for (unsigned i = 0; i < data.mNumObjs; ++i)
    {
        const AnimObjData &objData = data.mObjs[i];
        unsigned char frame =
                (unsigned char) ((float)objData.mNumFrames * animPct);
        frame = MIN(frame, objData.mNumFrames - 1);

        unsigned fontFrame = font.frames + 1;
        bool drawLetterOnTile = false;
        bool skipObject = false;
        if (params && params->mLetters && bg1)
        {
            if (i < GameStateMachine::getCurrentMaxLettersPerCube())
            {
                fontFrame = params->mLetters[i] - (int)'A';
                drawLetterOnTile = (fontFrame < font.frames);
                skipObject = !drawLetterOnTile;
            }
        }

        if (skipObject)
        {
            continue;
        }

        // clip to screen
        Vec2 pos(objData.mPositions[frame]);
        Vec2 clipOffset(0,0);
        Vec2 size(0, 0);
        unsigned assetFrames = 0;
        if (objData.mLayer == Layer_Sprite)
        {
            size = Vec2(objData.mSpriteAsset->width, objData.mSpriteAsset->height);
            assetFrames = objData.mSpriteAsset->frames;
        }
        else
        {
            ASSERT(objData.mAsset);
            size = Vec2(objData.mAsset->width, objData.mAsset->height);
            assetFrames = objData.mAsset->frames;
            // FIXME write utility AABB class
            if (pos.x >= MAX_ROWS || pos.y >= MAX_COLS)
            {
                continue; // totally offscreen
            }
            pos.x = MAX(pos.x, 0);
            pos.y = MAX(pos.y, 0);
            clipOffset = pos - objData.mPositions[frame];
            if (clipOffset.x >= (int)objData.mAsset->width ||
                clipOffset.y >= (int)objData.mAsset->height)
            {
                continue; // totally offscreen
            }
            size.x -= abs<int>(clipOffset.x);
            size.y -= abs<int>(clipOffset.y);
            size.x = MIN(size.x, MAX_ROWS - pos.x);
            size.y = MIN(size.y, MAX_COLS - pos.y);
        }
        ASSERT(size.x > 0);
        ASSERT(size.y > 0);
        // FIXME asset frame rate
        unsigned char assetFrame =
            MIN(assetFrames-1, (unsigned char) ((float)assetFrames * animPct));
#ifdef DEBUGzz
        switch (animT)
        {
        case AnimType_HintAppear:
        case AnimType_HintIdle:
        case AnimType_HintShake:
        case AnimType_HintDisappear:
        case AnimType_LockHint:
        case AnimType_LockedHint:
        case AnimType_NotWord:
            break;
        default:
            DEBUG_LOG(("anim cube ID: %d, anim type: %d, anim time: %f pct:%f frame: %d\n", params ? params->mCubeID : -1, animT, animTime, animPct, frame));
            break;
        }
#endif

        if (drawLetterOnTile && objData.mLayer == Layer_BG0)
        {
            Vec2 letterPos(pos);
            letterPos.y += 4; // TODO
            vid.BG0_drawPartialAsset(pos, clipOffset, size, *objData.mAsset, assetFrame);
            bg1->DrawPartialAsset(letterPos, Vec2(0,0), Vec2(size.x, font.height), font, fontFrame);
        }
        else if (objData.mLayer == Layer_BG0)
        {
            vid.BG0_drawPartialAsset(pos, clipOffset, size, *objData.mAltAsset, assetFrame);
        }
        else if (objData.mLayer == Layer_BG1)
        {
            bg1->DrawPartialAsset(pos, clipOffset, size, *objData.mAsset, assetFrame);
        }
        else
        {
            vid.moveSprite(0, objData.mPositions[frame]);
            vid.resizeSprite(0, size);
            vid.setSpriteImage(0, *objData.mSpriteAsset);
        }
    }

    if (params && params->mBorders)
    {
        // TODO fold border painting into the paint code
        const bool leftNeighbor = params ? params->mLeftNeighbor : false;
        const bool rightNeighbor = params ? params->mRightNeighbor : false;
        if (leftNeighbor || (rightNeighbor && animT != AnimType_NewWord && animT != AnimType_OldWord))
        {
            // don't draw left border
            vid.BG0_drawPartialAsset(Vec2(0, 14), Vec2(1, 0), Vec2(16, 2), BorderBottom);
        }
        else if (bg1)
        {
            // draw left border
            vid.BG0_drawPartialAsset(Vec2(0, 2), Vec2(0, 1), Vec2(2, 14), BorderLeft);
            bg1->DrawPartialAsset(Vec2(0, 1), Vec2(0, 0), Vec2(2, 1), BorderLeft);
            bg1->DrawPartialAsset(Vec2(1, 14), Vec2(0, 0), Vec2(1, 2), BorderBottom);
            vid.BG0_drawPartialAsset(Vec2(2, 14), Vec2(1, 0), Vec2(14, 2), BorderBottom);
        }

        if (rightNeighbor || (leftNeighbor && animT != AnimType_NewWord && animT != AnimType_OldWord))
        {
            // don't draw right border
            vid.BG0_drawPartialAsset(Vec2(0, 0), Vec2(0, 0), Vec2(16, 2), BorderTop);
        }
        else if (bg1)
        {
            // draw right border
            vid.BG0_drawPartialAsset(Vec2(14, 0), Vec2(0, 1), Vec2(2, 14), BorderRight);
            bg1->DrawPartialAsset(Vec2(14, 14), Vec2(0, 16), Vec2(2, 1), BorderRight);
            bg1->DrawPartialAsset(Vec2(14, 0), Vec2(16, 0), Vec2(1, 2), BorderTop);
            vid.BG0_drawPartialAsset(Vec2(0, 0), Vec2(1, 0), Vec2(14, 2), BorderTop);
        }


        const LevelProgressData &progressData =
                GameStateMachine::getInstance().getLevelProgressData();

        const static AssetImage *CheckMarkImagesBottom[] =
        {
            0,
            &BorderSlotBlank,
            &BorderSlotNormal,
            &BorderSlotBonus,

        };

        const static AssetImage *CheckMarkImagesTop[] =
        {
            0,
            &BorderSlotBlank,
            &BorderSlotNormal,
            &BorderSlotBonus,

        };

        const unsigned TopRowStartIndex = arraysize(progressData.mPuzzleProgress)/2;
        // this makes the icon bar not obvious enough
        //if (params && params->mCubeID == CUBE_ID_BASE)
        {
            for (unsigned i = 0; i < arraysize(progressData.mPuzzleProgress); ++i)
            {
                if (i < TopRowStartIndex)
                {
                    // row 1, bottom
                    const AssetImage *image =
                            CheckMarkImagesBottom[(int)progressData.mPuzzleProgress[i]];
                    if (image)
                    {
                        bg1->DrawAsset(Vec2(2 + i * 2, 14), *image);
                    }
                }
                else
                {
                    // row 2, top
                    const AssetImage *image =
                            CheckMarkImagesTop[(int)progressData.mPuzzleProgress[i]];
                    if (image)
                    {
                        bg1->DrawAsset(Vec2(2 + i * 2, 0), *image);
                    }
                }
            }
        }
    }

    // finished?
    return data.mLoop || animTime <= data.mDuration;
}


