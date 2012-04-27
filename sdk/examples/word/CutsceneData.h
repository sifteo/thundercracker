#ifndef CUTSCENEDATA_H
#define CUTSCENEDATA_H

#include "CutsceneIndex.h"

// FIXME generate from a spreadsheet?


const static bool CUTSCENE_ADVANCE_TO_NEXT[Num_CutsceneIndexes] =
{
    true,   // first run
    true,
    false,

    false, // first meta solve
    true,
    false,  // second meta solve

    true, // third solve
    true,
    true,

    true,
    true,
    false,
};

const static bool CUTSCENE_CITY_END[Num_CutsceneIndexes] =
{
    false,
    false,
    false,

    false,
    false,
    false,

    true,
    false,
    false,

    false,
    false,
    false,
};

#endif // CUTSCENEDATA_H
