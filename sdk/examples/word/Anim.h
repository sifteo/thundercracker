#ifndef ANIM_H
#define ANIM_H

#include <sifteo.h>
#include "LevelProgressData.h"
#include "Constants.h"

enum AnimType
{
    AnimType_NotWord,
    AnimType_SlideL,
    AnimType_SlideR,
    AnimType_OldWord,
    AnimType_NewWord,
    AnimType_EndofRound,
    AnimType_Shuffle,

    NumAnimTypes
};

struct AnimParams
{
    char mLetters[MAX_LETTERS_PER_CUBE + 1];
    bool mLeftNeighbor, mRightNeighbor;

};

bool animPaint(AnimType anim,
               VidMode_BG0_SPR_BG1 &vid,
               BG1Helper *bg1 = 0,
               float animTime = 0.f,
               const AnimParams *params=0);

#endif // ANIM_H
