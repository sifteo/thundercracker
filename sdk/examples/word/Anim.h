#ifndef ANIM_H
#define ANIM_H

#include <sifteo.h>
#include "LevelProgressData.h"
#include "Constants.h"

using namespace Sifteo;

enum AnimType
{
    AnimType_None = -1,

    AnimType_NotWord,
    AnimType_SlideL,
    AnimType_SlideR,
    AnimType_OldWord,
    AnimType_NewWord,
    AnimType_EndofRound,
    AnimType_Shuffle,
    AnimType_HintBarAppear,
    AnimType_HintBarIdle,
    AnimType_HintBarDisappear,
    AnimType_HintWindUpSlide,
    AnimType_HintSlideL,
    AnimType_HintSlideR,
    AnimType_NormalTilesEnter,
    AnimType_NormalTilesExit,
    AnimType_MetaTilesEnter,
    AnimType_MetaTilesShow,
    AnimType_MetaTilesExit,
    AnimType_MetaTilesReveal,
    AnimType_NormalTilesReveal, // reveal the letter on the just solved puzzle

    NumAnimTypes
};

struct SpriteParams
{
    Float2 mPositions[_SYS_VRAM_SPRITES];
    Float2 mEndPositions[_SYS_VRAM_SPRITES];
    float mStartDelay[_SYS_VRAM_SPRITES];
    float mSpeeds[_SYS_VRAM_SPRITES];
    bool mLoop[_SYS_VRAM_SPRITES];
};

struct AnimParams
{
    char mLetters[MAX_LETTERS_PER_CUBE + 1];
    bool mLeftNeighbor, mRightNeighbor;
    CubeID mCubeID;
    int mCubeAnim;
    unsigned char mMetaLetterIndex;
    unsigned char mLettersPerCube;
    bool mAllMetaLetters;
    SpriteParams *mSpriteParams;
};

bool animPaint(AnimType animT,
               VideoBuffer &vid,
               TileBuffer<16,16,1> &bg1TileBuf,
               float animTime,
               const AnimParams *params);

bool animHasNormalBorder(AnimType animT);

#endif // ANIM_H
