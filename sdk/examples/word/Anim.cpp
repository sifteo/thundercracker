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
    const Int2 *mPositions;
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
               VideoBuffer &vid,
               float animTime,
               const AnimParams *params)
{
    BG1Drawable & bg1 = vid.bg1;
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
                fmod(animTime, data.mDuration)/data.mDuration :
                MIN(1.f, animTime/data.mDuration);
    const int MAX_ROWS = 16, MAX_COLS = 16;
    for (unsigned i = 0; i < data.mNumObjs; ++i)
    {
        const AnimObjData &objData = data.mObjs[i];
        unsigned char frame =
                (unsigned char) ((float)objData.mNumFrames * animPct);
        frame = MIN(frame, objData.mNumFrames - 1);

        unsigned fontFrame = font.numFrames() + 1;
        bool drawLetterOnTile = false;
        bool blankLetterTile = false;
        bool metaLetterTile = false;
        if (params && params->mLetters && params->mLetters[0])
        {
            if (i < lettersPerCube)
            {
                fontFrame = params->mLetters[i] - (int)'A';
                drawLetterOnTile = (fontFrame < font.numFrames());
                blankLetterTile = !drawLetterOnTile;
                metaLetterTile =
                        !blankLetterTile &&
                        (params->mAllMetaLetters || params->mMetaLetterIndex == (int)i);
            }
        }

        // clip to screen
        Int2 pos(objData.mPositions[frame]);
        Int2 clipOffset = {0,0};
        Int2 size = {0, 0};
        unsigned assetFrames = 0;
        if (objData.mLayer == Layer_Sprite)
        {
            size = vec(objData.mSpriteAsset->pixelWidth(), objData.mSpriteAsset->pixelHeight());
            assetFrames =
                    (animT == AnimType_HintSlideL || animT == AnimType_HintSlideR) ?
                        MIN(4, objData.mSpriteAsset->numFrames()) : // TODO use the right indexes for left/right, with ping/pong
                        objData.mSpriteAsset->numFrames();
        }
        else
        {
            ASSERT(objData.mAsset);
            size = vec(objData.mAsset->tileWidth(), objData.mAsset->tileHeight());
            assetFrames = objData.mAsset->numFrames();
            // FIXME write utility AABB class
            if (pos.x >= MAX_ROWS || pos.y >= MAX_COLS)
            {
                continue; // totally offscreen
            }
            pos.x = MAX(pos.x, 0);
            pos.y = MAX(pos.y, 0);
            clipOffset = pos - objData.mPositions[frame];
            if (clipOffset.x >= (int)objData.mAsset->tileWidth() ||
                clipOffset.y >= (int)objData.mAsset->tileHeight())
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
            LOG("anim cube ID: %d, anim type: %d, anim time: %f pct:%f frame: %d\n", params ? params->mCubeID : -1, animT, animTime, animPct, frame);
            break;
        }
#endif

        if (objData.mLayer == Layer_BG0)
        {
            if (blankLetterTile)
            {
                vid.bg0.image(pos, size, *objData.mBlankLetterAsset, clipOffset, assetFrame);
//                 vid.bg0.drawPartialAsset(pos, clipOffset, size, *objData.mBlankLetterAsset, assetFrame);
            }
            else if (metaLetterTile)
            {
//     void image(UInt2 destXY, UInt2 size, const AssetImage &image, UInt2 srcXY, unsigned frame = 0)
                vid.bg0.image(pos, size, *objData.mMetaLetterAsset, clipOffset, assetFrame);
            }
            else
            {
                vid.bg0.image(pos, size, *objData.mAsset, clipOffset, assetFrame);
//                 vid.BG0_drawPartialAsset(pos, clipOffset, size, *objData.mAsset, assetFrame);
            }

            if (drawLetterOnTile && size.y > LETTER_Y_OFFSET)
            {
                Int2 letterPos(pos);
                letterPos.y += LETTER_Y_OFFSET; // TODO

                switch (animT)
                {
                case AnimType_NormalTilesReveal:
                    {
                        int sparkleRow = animPct * 12 + 2;
                        int sparkleOffset = sparkleRow - pos.y;
                        if (metaLetterTile)
                        {
                            if (sparkleOffset < size.y)
                            {
                                unsigned sparkleFrame =
                                        MIN(SparkleWipe.numFrames()-1, (unsigned char) ((float)SparkleWipe.numFrames() * animPct));
                             //   LOG(("sparkle frame %d\n", sparkleFrame));
                                bg1.image(vec(pos.x, sparkleRow),
                                                      vec(size.x, 1),
                                                      SparkleWipe,
                                                      vec(0,0),
                                                      sparkleFrame);
                                if (sparkleRow < letterPos.y + font.pixelHeight() - 1)
                                {
                                    bg1.image(vec(letterPos.x, sparkleRow + 1),
                                                  vec(size.x, letterPos.y + font.pixelHeight() - 1 - sparkleRow),
                                                  font,
                                                  vec(0, sparkleRow + 1 - letterPos.y),
                                                  fontFrame);
                                }
                            }
                        }
                        else
                        {
                            bg1.image(letterPos, vec(size.x, MIN(16 - letterPos.y, font.pixelHeight())), font, vec(0,0), fontFrame);
                        }
                    }
                    break;

                case AnimType_MetaTilesReveal:
                    {
                        bg1.image(letterPos, vec(size.x, MIN(16 - letterPos.y, font.pixelHeight())), font, vec(0,0), fontFrame);
                        int sparkleRow = (1.f - animPct) * 12 + 2;
                        unsigned char sparkleOffset = sparkleRow - pos.y;
                        if (i == params->mMetaLetterIndex && sparkleOffset < size.y)
                        {
                            bg1.image(vec(pos.x, sparkleRow),
                                              vec(size.x, 1),
                                              SparkleWipe,
                                              vec(0,0),
                                              MIN(SparkleWipe.numFrames()-1, (unsigned char) ((float)SparkleWipe.numFrames() * animPct)));
                            if (sparkleRow > letterPos.y)
                            {
                                bg1.image(letterPos,
                                              vec(size.x, sparkleRow - letterPos.y),
                                              font,
                                              vec(0, 0),
                                              ('Z' + 1) - 'A');
                            }
                        }
                    }
                break;

                default:
                    if (i == params->mMetaLetterIndex && animT == AnimType_MetaTilesEnter)
                    {
                        bg1.image(letterPos, vec(size.x, MIN(16 - letterPos.y, font.pixelHeight())), font, vec(0,0), 'Z' + 1 - 'A');

                    }
                    else if (!metaLetterTile || animT != AnimType_NormalTilesExit)
                    {
                        bg1.image(letterPos, vec(size.x, MIN(16 - letterPos.y, font.pixelHeight())), font, vec(0,0), fontFrame);
                    }
                    break;
                }


            }
        }
        else if (objData.mLayer == Layer_BG1)
        {
            bg1.image(pos, size, *objData.mAsset, clipOffset, assetFrame);
        }
        else // Layer_Sprite
        {
            vid.sprites[0].move(objData.mPositions[frame]);
            vid.sprites[0].resize(size);
            vid.sprites[0].setImage(*objData.mSpriteAsset, assetFrame);
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
        t = fmod(t, 1.0f);
        unsigned assetFrame = MIN(Sparkle.numFrames()-1, (unsigned)(t*((float)Sparkle.numFrames())));
        for (unsigned i=start; i<8; ++i)
        {
            if (params->mSpriteParams->mStartDelay[i] > 0.f)
            {
                vid.sprites[i].hide();
            }
            else
            {
                //LOG(("sparkle %d, (%d, %d), frame: %d, t: %f\n", i, pos.x, pos.y, assetFrame, t));
                vid.sprites[i].move(params->mSpriteParams->mPositions[i]);
                vid.sprites[i].resize(Sparkle.pixelWidth(), Sparkle.pixelHeight());
                vid.sprites[i].setImage(Sparkle, assetFrame);
            }
        }
    }

    if (params)
    {
        const static unsigned char NewWordBorderFrames[] =
        {
            1, 2, 3, 2, 1
        };
        const static unsigned char NewBonusWordBorderFrames[] =
        {
            4, 5, 6, 5, 4
        };
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
        bool isBonus = false;
        for (unsigned i = 0; i < arraysize(progressData.mPuzzleProgress); ++i)
        {
            if (i < TopRowStartIndex)
            {
                if (params->mCubeAnim == CubeAnim_Main)
                {
                    // row 1, bottom
                    const AssetImage *image =
                            CheckMarkImagesBottom[(int)progressData.mPuzzleProgress[i]];
                    if (image)
                    {
                        isBonus = (progressData.mPuzzleProgress[i] == CheckMarkState_CheckedBonus);
                        bg1.image(vec((unsigned)2 + i * 2, (unsigned)14), *image, MIN(image->numFrames()-1, 2));
                    }
                }
            }
            else
            {
                if (params->mCubeAnim == CubeAnim_Hint)
                {
                    unsigned hintIndex = i - TopRowStartIndex;
                    if (hintIndex < MAX_HINTS)
                    {
                        unsigned numHints = GameStateMachine::getInstance().getNumHints();
                        if (hintIndex  < numHints)
                        {
                            unsigned char assetFrames = (*CheckMarkImagesTop[2]).numFrames();
                            unsigned char assetFrame = 0;
                            if (animT == AnimType_HintWindUpSlide && hintIndex == numHints-1)
                            {
                                // loop X times
                                float f = fmod(animPct * 3.f, 1.f);
                                assetFrame = MIN(assetFrames-1, (unsigned char) ((float)f * assetFrames));
                            }

                            bg1.image(vec((unsigned)1 + hintIndex * 2, (unsigned)0), *CheckMarkImagesTop[2], assetFrame);
                        }
             /*           else
                        {
                            bg1.DrawAsset(vec(2 + (i - TopRowStartIndex) * 2, 0), *CheckMarkImagesTop[1]);
                        }
                        */
                    }
                }
            }
        }

        unsigned char bottomBorderFrame = 0;
        if (animT == AnimType_NewWord)
        {
            //const float ANIM_DURATION = 0.5f;
            float t = 2.f *animTime/data.mDuration;
            t = fmod(t, 1.0f);
            bottomBorderFrame =
                    (isBonus) ?
                        NewBonusWordBorderFrames[MIN(arraysize(NewBonusWordBorderFrames)-1, (unsigned)(t*((float)arraysize(NewBonusWordBorderFrames))))]:
                        NewWordBorderFrames[MIN(arraysize(NewWordBorderFrames)-1, (unsigned)(t*((float)arraysize(NewWordBorderFrames))))];
        }
        // TODO fold border painting into the paint code
        const bool leftNeighbor = params ? params->mLeftNeighbor : false;
        const bool rightNeighbor = params ? params->mRightNeighbor : false;
        const bool formsWord =
                (animT == AnimType_NewWord || animT == AnimType_OldWord);
        switch (params->mCubeAnim)
        {
        case CubeAnim_Main:
            // draw left border
            vid.bg0.image(vec(0, 2),
                         vec(2, 14),
                         (leftNeighbor || formsWord) ? BorderLeft : BorderLeftNoNeighbor,
                         vec(0, 1));
            bg1.image(vec(0, 1), vec(2, 1), BorderLeft, vec(0, 0));
            bg1.image(vec(1, 14), vec(1, 2), BorderBottom, vec(0, 0));
            vid.bg0.image(vec(2, 14), vec(14, 2), BorderBottom, vec(1, 0), bottomBorderFrame);

            // draw right border
            vid.bg0.image(vec(14, 0),
                         vec(2, 14),
                         (rightNeighbor || formsWord) ? BorderRight : BorderRightNoNeighbor,
                         vec(0, 1));
            bg1.image(vec(14, 14), vec(2, 1), BorderRight, vec(0, 16));
            bg1.image(vec(14, 0), vec(1, 2), BorderTop, vec(16, 0));
            vid.bg0.image(vec(0, 0), vec(14, 2), BorderTop, vec(1, 0));
            break;

        default:
            break;
        }

    }

    // finished?
    return data.mLoop || animTime <= data.mDuration;
}


