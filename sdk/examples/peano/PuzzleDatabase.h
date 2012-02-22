#pragma once

#include "PuzzleChapter.h"

namespace TotalsGame {

class PuzzleDatabase {

public:

    int NumChapters();
    PuzzleChapter *GetChapter(int index);
    int IndexOfChapter(PuzzleChapter *chapter);

    //rwrite as direct data access


private:
    PuzzleChapter *chapters;
    int numChapters;
};

}

