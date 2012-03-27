#include "SaveData.h"
#include "Game.h"

namespace TotalsGame 
{
SaveData::SaveData()
{
    for(int i = 0; i < 16; i++)
    {
        solvedPuzzles[i]=0;
    }
    solvedChapters = 0;
    hasDoneTutorial = false;
}

void SaveData::AddSolvedPuzzle(int chapter, int puzzle)
{
    if(chapter != -1 && puzzle != -1 && !IsPuzzleSolved(chapter, puzzle))
    {
        solvedPuzzles[chapter] |= 1 << puzzle;
        Save();
    }
}

void SaveData::AddSolvedChapter(int chapter)
{
    if(chapter != -1)
    {
        solvedChapters |= 1 << chapter;
        Save();
    }

}

void SaveData::Reset()
{
    delete Game::currentPuzzle;
    delete Game::previousPuzzle;
    Game::currentPuzzle = NULL;
    Game::previousPuzzle = NULL;
    Save();
}

void SaveData::Save()
{
    //TODO savedata save
}

void SaveData::Load()
{
    //todo savedata load
}

Puzzle *SaveData::FindNextPuzzle()
{
    for(int chapter = 0; chapter < Database::NumChapters(); chapter++)
    {
        if (!IsChapterSolved(chapter))
        {
            for(int puzzle = 0; puzzle < Database::NumPuzzlesInChapter(chapter); puzzle++)
            {
                if(!IsPuzzleSolved(chapter, puzzle) && Database::NumTokensInPuzzle(chapter, puzzle) <= NUM_CUBES)
                {
                    return Database::GetPuzzleInChapter(chapter, puzzle);
                }
            }
        }
    }
    return NULL;
}

bool SaveData::AllChaptersSolved()
{
    for(int chapter = 0; chapter < Database::NumChapters(); chapter++)
    {
        if(Database::CanBePlayedWithCurrentCubeSet(chapter) && !IsChapterSolved(chapter))
        {
            return false;
        }
    }

    return true;
}

bool SaveData::IsChapterUnlockedWithCurrentCubeSet(int i)
{
    // the first chapter is always unlocked
    if (i == 0) { return true; }

    // if the chapter has been solved
    if (IsChapterSolved(i)) { return true; }

    // if the previous chapter (subject to the current cubeset) has been solved
    int j = i-1;
    while(j > 0 && !Database::CanBePlayedWithCurrentCubeSet(j))
    {
        --j;
    }
    return IsChapterSolved(j);
}

bool SaveData::IsPuzzleSolved(int chapter, int puzzle)
{


    if(chapter != -1 && puzzle != -1)
    {
        return solvedPuzzles[chapter] & 1 << puzzle;

    }
    return false;

}

bool SaveData::IsChapterSolved(int chapter)
{
    if(chapter != -1)
        return solvedChapters & 1 << chapter;
    return false;
}

void SaveData::CompleteTutorial()
{
    if (!hasDoneTutorial) {
        hasDoneTutorial = true;
        Save();
    }
}

bool SaveData::HasCompletedTutorial()
{
    return hasDoneTutorial;
}


}

