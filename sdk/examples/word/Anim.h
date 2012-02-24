#ifndef ANIM_H
#define ANIM_H

#include <sifteo.h>

enum AnimIndex
{
    AnimIndex_Tile2Idle,
    AnimIndex_Tile2SlideL,
    AnimIndex_Tile2SlideR,
    AnimIndex_Tile2Glow,

    NumAnimIndexes
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

bool animPaint(AnimIndex anim,
               VidMode_BG0_SPR_BG1 &vid,
               BG1Helper *bg1 = 0,
               float animTime = 0.f,
               const AnimParams *params=0);

#endif // ANIM_H
