#ifndef CUTSCENEPAINTDATA_H
#define CUTSCENEPAINTDATA_H

#include "CutsceneIndex.h"

const static AssetImage* CUTSCENE_IMAGES[Num_CutsceneIndexes] =
{
    &Scene01a,
    &Scene01b,
    &Scene01c,
    &Scene02a,
    &Scene02b,
    &Scene02c,
    &Scene03a,
    &Scene03b,
    &Scene03c,
    &Scene04a,
    &Scene04b,
    &Scene04c,
};

const static char* CUTSCENE_DIALOGUE[Num_CutsceneIndexes] =
{
    "You've arrived in\nLondon, England",
    "You're staying with an\nold friend this summer,\n Doc. Henry Bhat.",
    "What's this?",
    "Hey, kiddo! Bad news. I lost\nmy job at the museum... Hold on!",
    "Test",
    "Test",
    "Test",
    "Test",
    "Test",
    "Test",
    "Test",
    "Test",
};

const static bool CUTSCENE_DIALOGUE_TOP[Num_CutsceneIndexes] =
{
    false,
    false,
    true,

    false,
    false,
    false,

    false,
    true,
    false,

    true,
    false,
    false,
};

#endif // CUTSCENEPAINTDATA_H
