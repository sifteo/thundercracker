
#include "PuzzleDatabase.h"

namespace TotalsGame {
  
      
    int PuzzleDatabase::NumChapters()
    {
        return numChapters;
    }
    
    PuzzleChapter *PuzzleDatabase::GetChapter(int index)
    {
        return chapters + index;
    }
    
    int PuzzleDatabase::IndexOfChapter(PuzzleChapter *chapter)
    {
        for(int i = 0; i < numChapters; i++)
        {
            if(chapter == &chapters[i])
                return i;
        }
        return -1;
    }
      
  
}

