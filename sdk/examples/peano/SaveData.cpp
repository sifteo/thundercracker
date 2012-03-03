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
            SaveData();
        }
    }
    
    void SaveData::AddSolvedChapter(int chapter)
                                    {
                                        if(chapter != -1)
                                            solvedChapters |= 1 << chapter;
                                
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
        //TODO
    }
    
    bool SaveData::IsPuzzleSolved(int chapter, int puzzle)
    {return true;//TODO
        

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
    

}

