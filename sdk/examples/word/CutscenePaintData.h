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
    "Doc's' old trunk begins\nto glow. You decide to\nopen it.",

    "Hey, kiddo! Bad news.\nI lost my job at\nthe museum... Hold on!",
    "You cracked the code!\nThat box is from the\nancient Silk Road era!",
    "If we can figure out the\nmystery, we may\nfind a great treasure!",

    "Brilliant work! With you\nby my side I can prove\nmyself to the board!",
    "I know exactly where we\nneed to go! Race\n you to the boat!",
    NULL,

    "Wecome to the great\ncity of Athens,\ncapital of Greece!",
    "And this is the\nParhenon! We'll find\nour next clue here!",
    "Ah ha! Here it is the\nnext set of clues!\nDo your thing!",
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
