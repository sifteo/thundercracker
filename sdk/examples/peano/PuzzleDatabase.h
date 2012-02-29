#pragma once

#include "config.h"

#if !DISABLE_CHAPTERS

#include "Puzzle.h"
//#include "Guid.h"

namespace TotalsGame
{
namespace Database
{
    int NumChapters();

    int NumPuzzlesInChapter(int chapter);
    int NumTokensInPuzzle(int chapter, int puzzle);
//    Guid GuidForPuzzle(int chapter, int puzzle);
    const AssetImage &ImageForChapter(int chapter);
    const char *NameOfChapter(int chapter);
    Puzzle *GetPuzzleInChapter(int chapter, int puzzle);

      bool CanBePlayedWithCurrentCubeSet(int chapter);
      int FirstPuzzleForCurrentCubeSetInChapter(int chapter);
}
}

#endif // !DISABLE_CHAPTERS
