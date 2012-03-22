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
    const Byte2 *mPositions;
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
const static Byte2 positions[] =
{
    { 2, 2 },
    { 8, 2 },
    { 7, 2 },
    { 6, 2 },
    { 5, 2 },
    { 4, 2 },
    { 3, 2 },
    { 2, 2 },
    { 2, 2 },
    { 2, 2 },
    { 2, 2 }, // [10]
    { 3, 2 },
    { 4, 2 },
    { 5, 2 },
    { 6, 2 },
    { 7, 2 },
    { 8, 2 },
    { 8, 2 },
    { 8, 2 },
    { 8, 2 },
    { 7, 2 }, // [20]
    { 6, 2 },
    { 5, 2 },
    { 4, 2 },
    { 3, 2 },
    { 2, 2 },
    { 3, 3 }, // [26]
    { 56, 16 }, // [27]
    { 24, 16 }, // [28]
    { 88, 16 }, // [29]
    { 86, 16 }, // [30]2
    { 84, 16 }, // [31]2
    { 80, 16 }, // [32]4
    { 72, 16 }, // [33]8
    { 64, 16 }, // [34]8
    { 56, 16 }, // [35]8
    { 48, 16 }, // [36]8
    { 40, 16 }, // [29]8
    { 36, 16 }, // [30]4
    { 32, 16 }, // [31]4
    { 30, 16 }, // [32]2
    { 28, 16 }, // [33]2
    { 26, 16 }, // [34]2
    { 25, 16 }, // [35]1
    { 24, 16 }, // [36]1
    { 24, 16 }, // [29]
    { 26, 16 }, // [30]2
    { 28, 16 }, // [31]2
    { 32, 16 }, // [32]4
    { 40, 16 }, // [33]8
    { 48, 16 }, // [34]8
    { 56, 16 }, // [35]8
    { 64, 16 }, // [36]8
    { 72, 16 }, // [29]8
    { 76, 16 }, // [30]4
    { 80, 16 }, // [31]4
    { 82, 16 }, // [32]2
    { 84, 16 }, // [33]2
    { 86, 16 }, // [34]2
    { 87, 16 }, // [35]1
    { 88, 16 }, // [36]1
    { 2, 2 }, // [37]
    { 6, 2 }, // [38]
    { 8, 2 }, // [39]
    { 12, 2 }, // [40]
    { 2, 11 }, // [41]
    { 6, 11 }, // [42]
    { 8, 11 }, // [43]
    { 12, 11 }, // [44]
    { 53, 2 }, // [45]
    { 59, 2 }, // [46]
    { 52, 2 }, // [47]
    { 60, 2 }, // [48]
    { 54, 2 }, // [49]
    { 58, 2 }, // [50]
    { 54, 2 }, // [51]
    { 58, 2 }, // [52]
    { 56, 2 }, // [53]
    { 58, 2 }, // [54]2
    { 60, 2 }, // [55]2
    { 64, 2 }, // [56]4
    { 72, 2 }, // [57]8
    { 80, 2 }, // [58]8
    { 84, 2 }, // [59]4
    { 88, 2 }, // [60]4
};

const static AnimObjData animObjData[] =
{    
    {&Tile2, &Tile2Blank, 0, Layer_BG0, 0x0, 1, &positions[0]},// AnimType_NotWord
    {&Tile2, &Tile2Blank, 0, Layer_BG0, 0x0, 1, &positions[1]},
    {&Tile2, &Tile2Blank, 0, Layer_BG0, 0x0, 10, &positions[7]}, // AnimType_SlideL
    {&Tile2, &Tile2Blank, 0, Layer_BG0, 0x0, 7, &positions[2]},
    {&Tile2, &Tile2Blank, 0, Layer_BG0, 0x0, 7, &positions[10]}, // AnimType_SlideR
    {&Tile2, &Tile2Blank, 0, Layer_BG0, 0x0, 10, &positions[16]},
    {&Tile2Glow, &Tile2Blank, 0, Layer_BG0, 0x0, 1, &positions[0]}, // AnimType_OldWord
    {&Tile2Glow, &Tile2Blank, 0, Layer_BG0, 0x0, 1, &positions[1]},
    {&LevelComplete , &LevelComplete, 0, Layer_BG1, 0x0, 1, &positions[26]}, // CityProgression
    { 0, 0, &HintSprite, Layer_Sprite, 0x0, 1, &positions[27]}, // HintIdle
    { 0, 0, &HintSprite, Layer_Sprite, 0x0, 1, &positions[28]}, // HintLocked
    { 0, 0, &HintSprite, Layer_Sprite, 0x0, 16, &positions[29]}, // AnimType_HintSlideL
    { 0, 0, &HintSprite, Layer_Sprite, 0x0, 16, &positions[45]}, // AnimType_HintSlideR
    { 0, 0, &HintSprite, Layer_Sprite, 0x0, 8, &positions[69]}, // HintShake
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
    { 0.5f, true, 2, &animObjData[0]},
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

    // AnimType_NotWord
    { 1.f, true, 2, &animObjData[0]},
    // AnimType_SlideL
    { 0.5f, false, 2, &animObjData[2]},
    // AnimType_SlideR
    { 0.5f, false, 2, &animObjData[4]},
    // AnimType_OldWord
    { 1.f, true, 2, &animObjData[6]},
    //AnimType_NewWord,
    { 1.5f, true, 2, &animObjData[6]},
    //AnimType_EndOfRound,
    { 1.f, true, 2, &animObjData[0]},
    //AnimType_Shuffle,
    { 0.5f, true, 2, &animObjData[0]},
    //AnimType_CityProgression
    { 1.f, true, 1, &animObjData[8]},
    //AnimType_HintBarAppear,
    { 1.f, true, 1, &animObjData[0]},
    //AnimType_HintBarIdle,
    { 3.f, false, 1, &animObjData[9]},
    //AnimType_HintBarDisappear,
    { 0.3f, false, 1, &animObjData[13]},
    //AnimType_HintWindUpSlide,
    { 2.0f, false, 1, &animObjData[13]},
    // AnimIndex_HintSlideL
    { 1.0f, true, 1, &animObjData[11]},
    // AnimIndex_HintSlideR
    { 1.0f, true, 1, &animObjData[12]},
    //AnimType_HintNeighborL
    { 1.f, true, 2, &animObjData[0]},
    //AnimType_HintNeighborR
    { 1.f, true, 6, &animObjData[13]},
    //AnimType_LockedHintOldWord,
    { 1.f, true, 6, &animObjData[13]},

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
    { 0.5f, true, 2, &animObjData[0]},
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
    const static AssetImage* fontsGlow[] =
    {
        &Font1Letter, &Font2LetterGlow, &Font3Letter,
    };
    const AssetImage& font =
            (animT == AnimType_NewWord || animT == AnimType_OldWord) ?
                *fontsGlow[GameStateMachine::getCurrentMaxLettersPerCube() - 1] :
                *fonts[GameStateMachine::getCurrentMaxLettersPerCube() - 1];

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
                fmod(animTime, data.mDuration)/data.mDuration :
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
        bool blankLetterTile = false;
        if (params && params->mLetters && params->mLetters[0] && bg1)
        {
            if (i < GameStateMachine::getCurrentMaxLettersPerCube())
            {
                fontFrame = params->mLetters[i] - (int)'A';
                drawLetterOnTile = (fontFrame < font.frames);
                blankLetterTile = !drawLetterOnTile;
            }
        }

        // clip to screen
        Int2 pos = objData.mPositions[frame];
        Int2 clipOffset = {0, 0};
        Int2 size = {0, 0};
        unsigned assetFrames = 0;
        if (objData.mLayer == Layer_Sprite)
        {
            size = Vec2(objData.mSpriteAsset->width * 8, objData.mSpriteAsset->height * 8);
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
            clipOffset = pos - objData.mPositions[frame].toInt();
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

        if (objData.mLayer == Layer_BG0)
        {
            if (blankLetterTile)
            {
                vid.BG0_drawPartialAsset(pos, clipOffset, size, *objData.mAltAsset, assetFrame);
            }
            else
            {
                vid.BG0_drawPartialAsset(pos, clipOffset, size, *objData.mAsset, assetFrame);
            }

            if (drawLetterOnTile)
            {
                Int2 letterPos = pos;
                letterPos.y += 5; // TODO
                bg1->DrawPartialAsset(letterPos, Vec2(0,0), Vec2<int>(size.x, font.height), font, fontFrame);
            }
        }
        else if (objData.mLayer == Layer_BG1)
        {
            bg1->DrawPartialAsset(pos, clipOffset, size, *objData.mAsset, assetFrame);
        }
        else // Layer_Sprite
        {
            vid.moveSprite(0, objData.mPositions[frame]);
            vid.resizeSprite(0, size);
            vid.setSpriteImage(0, *objData.mSpriteAsset, assetFrame);
        }
    }

    // do procedural sprite stuff, it may not be a good fit for the data
    // driven approach
    if (params && params->mSpriteParams)
    {
        float t = 4.f * animTime/data.mDuration;
        t = fmod(t, 1.0f);
        unsigned assetFrame = MIN(Sparkle.frames-1, (unsigned)(t*((float)Sparkle.frames)));
        for (unsigned i=1; i<8; ++i)
        {
            //DEBUG_LOG(("sparkle %d, (%d, %d), frame: %d, t: %f\n", i, pos.x, pos.y, assetFrame, t));
            vid.moveSprite(i, params->mSpriteParams->mPositions[i]);
            vid.resizeSprite(i, Sparkle.width * 8, Sparkle.height * 8);
            vid.setSpriteImage(i, Sparkle, assetFrame);
        }
    }

    if (params && params->mBorders)
    {
        const static unsigned char NewWordBorderFrames[] =
        {
            1, 2, 3, 2, 1
        };
        const static unsigned char NewBonusWordBorderFrames[] =
        {
            4, 5, 6, 5, 4
        };
        unsigned char bottomBorderFrame = 0;
        if (animT == AnimType_NewWord)
        {
            //const float ANIM_DURATION = 0.5f;
            float t = 2.f *animTime/data.mDuration;
            t = fmod(t, 1.0f);
            bottomBorderFrame =
                    (params->mBonus) ?
                        NewBonusWordBorderFrames[MIN(arraysize(NewBonusWordBorderFrames)-1, (unsigned)(t*((float)arraysize(NewBonusWordBorderFrames))))]:
                        NewWordBorderFrames[MIN(arraysize(NewWordBorderFrames)-1, (unsigned)(t*((float)arraysize(NewWordBorderFrames))))];
        }
        // TODO fold border painting into the paint code
        const bool leftNeighbor = params ? params->mLeftNeighbor : false;
        const bool rightNeighbor = params ? params->mRightNeighbor : false;
        const bool formsWord =
                (animT == AnimType_NewWord || animT == AnimType_OldWord);
        if (false && (leftNeighbor || (rightNeighbor && !formsWord)))
        {
            // don't draw left border
            vid.BG0_drawPartialAsset(Vec2(0, 14), Vec2(1, 0), Vec2(16, 2), BorderBottom, bottomBorderFrame);
        }
        else if (bg1)
        {
            // draw left border
            vid.BG0_drawPartialAsset(Vec2(0, 2),
                                     Vec2(0, 1),
                                     Vec2(2, 14),
                                     (leftNeighbor || formsWord) ?
                                         BorderLeft :
                                         BorderLeftNoNeighbor);
            bg1->DrawPartialAsset(Vec2(0, 1), Vec2(0, 0), Vec2(2, 1), BorderLeft);
            bg1->DrawPartialAsset(Vec2(1, 14), Vec2(0, 0), Vec2(1, 2), BorderBottom);
            vid.BG0_drawPartialAsset(Vec2(2, 14), Vec2(1, 0), Vec2(14, 2), BorderBottom, bottomBorderFrame);
        }

        if (false && (rightNeighbor || (leftNeighbor && !formsWord)))
        {
            // don't draw right border
            vid.BG0_drawPartialAsset(Vec2(0, 0), Vec2(0, 0), Vec2(16, 2), BorderTop);
        }
        else if (bg1)
        {
            // draw right border
            vid.BG0_drawPartialAsset(Vec2(14, 0),
                                     Vec2(0, 1),
                                     Vec2(2, 14),
                                     (rightNeighbor || formsWord) ?
                                         BorderRight :
                                         BorderRightNoNeighbor);
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
            0,
            &BorderSlotHint,
            0,

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
                        bg1->DrawAsset(Vec2<int>(2 + i * 2, 14), *image);
                    }
                }
                else
                {
                    if (i - TopRowStartIndex < MAX_HINTS)
                    {
                        if (i - TopRowStartIndex  < GameStateMachine::getInstance().getNumHints())
                        {
                            bg1->DrawAsset(Vec2<int>(2 + (i - TopRowStartIndex) * 2, 0), *CheckMarkImagesTop[2]);
                        }
             /*           else
                        {
                            bg1->DrawAsset(Vec2(2 + (i - TopRowStartIndex) * 2, 0), *CheckMarkImagesTop[1]);
                        }
                        */
                    }
                }
            }
        }
    }

    // finished?
    return data.mLoop || animTime <= data.mDuration;
}


