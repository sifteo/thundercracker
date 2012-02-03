#include "PuzzleChapter.h"
#include "Game.h"

namespace TotalsGame
{
    
    bool PuzzleChapter::HasBeenSolved() 
    {
        return guid != Guid::Empty && Game::GetInstance().saveData.IsSolved(guid);
    }
    
    bool PuzzleChapter::CanBePlayedWithCurrentCubeSet() 
    {
        return NULL != FirstPuzzleForCurrentCubeSet();
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