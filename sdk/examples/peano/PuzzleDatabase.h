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
      static const int MAX_CHAPTERS = 16;
      PuzzleChapter chapters[MAX_CHAPTERS];
      int numChapters;
  };
  
}

