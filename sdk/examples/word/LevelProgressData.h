#ifndef LEVELPROGRESSDATA_H
#define LEVELPROGRESSDATA_H

enum CheckMarkState
{
    CheckMarkState_Hidden,
    CheckMarkState_Unchecked,
    CheckMarkState_Checked,
    CheckMarkState_CheckedBonus,
};

struct LevelProgressData
{
    CheckMarkState mPuzzleProgress[12];  // 12 max, 6 on top, 6 on bottom
    // TODO add metapuzzle progress
    //float mTimeLeft;
};

#endif // LEVELPROGRESSDATA_H
