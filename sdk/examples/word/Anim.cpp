#include "Anim.h"
#include "assets.gen.h"
#include "Dictionary.h"
#include "GameStateMachine.h"

enum Layer
{
    Layer_BG0,
    Layer_Sprite,
    Layer_BG1,
};

struct AnimObjData
{
    const AssetImage *mAsset;
    const AssetImage *mBlankLetterAsset;
    const AssetImage *mMetaLetterAsset;
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

// include Python generated static arrays of the above types
#include "AnimData.h"


bool animPaint(AnimType animT,
               VidMode_BG0_SPR_BG1 &vid,
               BG1Helper *bg1,
               float animTime,
               const AnimParams *params)
{
    const int LETTER_Y_OFFSET = 5;
    unsigned char lettersPerCube = params ? params->mLettersPerCube : 1;
    lettersPerCube = clamp<unsigned>(lettersPerCube, 1, MAX_LETTERS_PER_CUBE);
    const static AssetImage* fonts[] =
    {
        &Font1Letter, &Font2Letter, &Font3Letter,
    };
    const static AssetImage* fontsGlow[] =
    {
        &Font1LetterGlow, &Font2LetterGlow, &Font3LetterGlow,
    };
    const AssetImage& font =
            (animT == AnimType_NewWord || animT == AnimType_OldWord) ?
                *fontsGlow[lettersPerCube - 1] :
                *fonts[lettersPerCube - 1];

    if (animT == AnimType_None)
    {
        return false;
    }
    unsigned anim = (animT + NumAnimTypes * (lettersPerCube - 1));
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
        bool blankLetterTile = false;
        bool metaLetterTile = false;
        if (params && params->mLetters && params->mLetters[0] && bg1)
        {
            if (i < lettersPerCube)
            {
                fontFrame = params->mLetters[i] - (int)'A';
                drawLetterOnTile = (fontFrame < font.frames);
                blankLetterTile = !drawLetterOnTile;
                metaLetterTile =
                        !blankLetterTile &&
                        (params->mAllMetaLetters || params->mMetaLetterIndex == (int)i);
            }
        }

        // clip to screen
        Vec2 pos(objData.mPositions[frame]);
        Vec2 clipOffset(0,0);
        Vec2 size(0, 0);
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
#ifdef DEBUGz
        switch (animT)
        {
        case AnimType_NotWord:
        case AnimType_NewWord:
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
                vid.BG0_drawPartialAsset(pos, clipOffset, size, *objData.mBlankLetterAsset, assetFrame);
            }
            else if (metaLetterTile)
            {
                vid.BG0_drawPartialAsset(pos, clipOffset, size, *objData.mMetaLetterAsset, assetFrame);
            }
            else
            {
                vid.BG0_drawPartialAsset(pos, clipOffset, size, *objData.mAsset, assetFrame);
            }

            if (drawLetterOnTile && size.y > LETTER_Y_OFFSET)
            {
                Vec2 letterPos(pos);
                letterPos.y += LETTER_Y_OFFSET; // TODO

                switch (animT)
                {
                case AnimType_NormalTilesReveal:
                    {
                        unsigned char sparkleRow = animPct * 12 + 2;
                        unsigned char sparkleOffset = sparkleRow - pos.y;
                        if (metaLetterTile)
                        {
                            if (sparkleOffset < size.y)
                            {
                                bg1->DrawPartialAsset(Vec2(pos.x, sparkleRow),
                                                      Vec2(0,0),
                                                      Vec2(size.x, 1),
                                                      SparkleWipe,
                                                      MIN(SparkleWipe.frames-1, (unsigned char) ((float)SparkleWipe.frames * animPct)));
                                if (sparkleRow < letterPos.y + font.height - 1)
                                {
                                    bg1->DrawPartialAsset(Vec2(letterPos.x, sparkleRow + 1),
                                                          Vec2(0, sparkleRow + 1 - letterPos.y),
                                                          Vec2(size.x, letterPos.y + font.height - 1 - sparkleRow),
                                                          font,
                                                          fontFrame);
                                }
                            }
                        }
                        else
                        {
                            bg1->DrawPartialAsset(letterPos, Vec2(0,0), Vec2(size.x, MIN(16 - letterPos.y, font.height)), font, fontFrame);
                        }
                    }
                    break;

                case AnimType_MetaTilesReveal:
                    {
                        bg1->DrawPartialAsset(letterPos, Vec2(0,0), Vec2(size.x, MIN(16 - letterPos.y, font.height)), font, fontFrame);
                        unsigned char sparkleRow = (1.f - animPct) * 12 + 2;
                        unsigned char sparkleOffset = sparkleRow - pos.y;
                        if (i == params->mMetaLetterIndex && sparkleOffset < size.y)
                        {
                            bg1->DrawPartialAsset(Vec2(pos.x, sparkleRow),
                                                  Vec2(0,0),
                                                  Vec2(size.x, 1),
                                                  SparkleWipe,
                                                  MIN(SparkleWipe.frames-1, (unsigned char) ((float)SparkleWipe.frames * animPct)));
                            if (sparkleRow > letterPos.y)
                            {
                                bg1->DrawPartialAsset(letterPos,
                                                      Vec2(0, 0),
                                                      Vec2(size.x, sparkleRow - letterPos.y),
                                                      font,
                                                      ('Z' + 1) - 'A');
                            }
                        }
                    }
                break;

                default:
                    if (!metaLetterTile || animT != AnimType_NormalTilesExit)
                    {
                        bg1->DrawPartialAsset(letterPos, Vec2(0,0), Vec2(size.x, MIN(16 - letterPos.y, font.height)), font, fontFrame);
                    }
                    break;
                }


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
        float t = animTime/data.mDuration;
        unsigned start = 0;
        if (animT == AnimType_NewWord)
        {
            t *= 4.f;
            start = 1;
        }
        t = fmodf(t, 1.0f);
        unsigned assetFrame = MIN(Sparkle.frames-1, (unsigned)(t*((float)Sparkle.frames)));
        for (unsigned i=start; i<8; ++i)
        {
            if (params->mSpriteParams->mStartDelay[i] > 0.f)
            {
                vid.hideSprite(i);
            }
            else
            {
                //DEBUG_LOG(("sparkle %d, (%d, %d), frame: %d, t: %f\n", i, pos.x, pos.y, assetFrame, t));
                vid.moveSprite(i, params->mSpriteParams->mPositions[i]);
                vid.resizeSprite(i, Sparkle.width * 8, Sparkle.height * 8);
                vid.setSpriteImage(i, Sparkle, assetFrame);
            }
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
            t = fmodf(t, 1.0f);
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
        switch (animT)
        {
        default:
            break;

        case AnimType_HintBarAppear:
        case AnimType_HintBarIdle:
        case AnimType_HintBarDisappear:
        case AnimType_HintWindUpSlide:
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
                    unsigned hintIndex = i - TopRowStartIndex;
                    if (hintIndex < MAX_HINTS)
                    {
                        unsigned numHints = GameStateMachine::getInstance().getNumHints();
                        if (hintIndex  < numHints)
                        {
                            unsigned char assetFrames = (*CheckMarkImagesTop[2]).frames;
                            unsigned char assetFrame =
                                    (animT == AnimType_HintWindUpSlide && hintIndex == numHints-1) ?
                                        MIN(assetFrames-1, (unsigned char) ((float)assetFrames * animPct)) :
                                        0;

                            bg1->DrawAsset(Vec2(1 + hintIndex * 2, 0), *CheckMarkImagesTop[2], assetFrame);
                        }
             /*           else
                        {
                            bg1->DrawAsset(Vec2(2 + (i - TopRowStartIndex) * 2, 0), *CheckMarkImagesTop[1]);
                        }
                        */
                    }
                }
            }
            break;
        }
    }

    // finished?
    return data.mLoop || animTime <= data.mDuration;
}


