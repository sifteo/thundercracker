#pragma once

#include "Puzzle.h"
#include "Guid.h"

namespace TotalsGame
{
namespace Database
{
    int NumChapters();

    int NumPuzzlesInChapter(int chapter);
    int NumTokensInPuzzle(int chapter, int puzzle);
    Guid GuidForPuzzle(int chapter, int puzzle);
    const PinnedAssetImage &ImageForChapter(int chapter);
    const char *NameOfChapter(int chapter);
    Puzzle *GetPuzzleInChapter(int chapter, int puzzle);

      bool HasPuzzleBeenSolved(int chapter, int puzzle);
      bool CanBePlayedWithCurrentCubeSet(int chapter);
      void SavePuzzleAsSolved(int chapter, int puzzle);
      void SaveChapterAsSolved(int chapter);
      int FirstPuzzleForCurrentCubeSetInChapter(int chapter);
}
}

