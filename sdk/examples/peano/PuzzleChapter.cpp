#include "PuzzleChapter.h"
#include "Game.h"

namespace TotalsGame
{

PuzzleChapter::PuzzleChapter()
{
    db = NULL;
    idImage = NULL;
    name = "";
    numPuzzles = 0;
}

int PuzzleChapter::NumPuzzles() { return numPuzzles;}

bool PuzzleChapter::HasBeenSolved()
{
    return guid != Guid::Empty && Game::GetInstance().saveData.IsSolved(guid);
}

bool PuzzleChapter::CanBePlayedWithCurrentCubeSet()
{
    return NULL != FirstPuzzleForCurrentCubeSet();
}

Puzzle *PuzzleChapter::GetPuzzle(int index)
{
    ASSERT(index < numPuzzles);
    if(index < numPuzzles)
        return puzzles+index;
    else
        return NULL;
}

int PuzzleChapter::IndexOfPuzzle(Puzzle *p)
{
    for(int i = 0; i < numPuzzles; i++)
    {
        if(puzzles+i == p)
            return i;
    }
    return -1;
}

void PuzzleChapter::SaveAsSolved()
{
    if (guid != Guid::Empty) {
        Game::GetInstance().saveData.AddSolved(guid);
        Game::GetInstance().saveData.Save();
    }
}

Puzzle *PuzzleChapter::FirstPuzzleForCurrentCubeSet()
{
    for(int i = 0; i < numPuzzles; i++)
    {
        if(puzzles[i].GetNumTokens() <= Game::NUMBER_OF_CUBES)
            return puzzles + i;
    }

    return NULL;
}

}
