#ifndef ANIM_H
#define ANIM_H

#include <sifteo.h>

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


enum CheckMarkState
{
    CheckMarkState_Unchecked,
    CheckMarkState_Checked,
    CheckMarkState_CheckedBonus,
};

struct AnimParams
{
    const char *mLetters;
    CheckMarkState mProgState[16];
    //float mTimeLeft;
};

bool animPaint(AnimType anim,
               VidMode_BG0_SPR_BG1 &vid,
               BG1Helper *bg1 = 0,
               float animTime = 0.f,
               const AnimParams *params=0);

#endif // ANIM_H
