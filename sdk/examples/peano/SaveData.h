#pragma once

#include "sifteo.h"
//#include "Guid.h"

namespace TotalsGame 
{

class Puzzle;

class SaveData
{
public:
    SaveData();

    void AddSolvedPuzzle(int chapter, int puzzle);
    void AddSolvedChapter(int chapter);
    void Save();
    void Load();

    bool IsPuzzleSolved(int chapter, int puzzle);
    bool IsChapterSolved(int chapter);
    bool AllChaptersSolved();
    void CompleteTutorial();
    void Reset();

    bool IsChapterUnlockedWithCurrentCubeSet(int i);
    Puzzle *FindNextPuzzle();

private:
    
    uint16_t solvedPuzzles[16];
    uint16_t solvedChapters;
    bool hasDoneTutorial;
};
}

