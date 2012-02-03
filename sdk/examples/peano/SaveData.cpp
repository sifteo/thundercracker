#include "SaveData.h"

namespace TotalsGame 
{
    SaveData::SaveData()
    {
        numSolvedGuids = 0;
    }
    
    void SaveData::AddSolved(const Guid &guid)
    {
        if(!IsSolved(guid))
        {
            solvedGuids[numSolvedGuids] = guid;
            numSolvedGuids++;
        }
    }
    
    void SaveData::Save() 
    {
        //TODO
    }
    
    bool SaveData::IsSolved(const Guid &guid)
    {
        for(int i = 0; i < numSolvedGuids; i++)
        {
            if(solvedGuids[i] == guid)
                return true;
        }
        return false;
        
    }
    

}

