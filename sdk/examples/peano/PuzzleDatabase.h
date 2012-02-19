#pragma once

#include "PuzzleChapter.h"

namespace TotalsGame {

class PuzzleDatabase {

public:

    static const int MAX_CHAPTERS = 16;

    int NumChapters();
    PuzzleChapter *GetChapter(int index);
    int IndexOfChapter(PuzzleChapter *chapter);

    //rwrite as direct data access


private:
    PuzzleChapter chapters[MAX_CHAPTERS];
    int numChapters;
};

}

