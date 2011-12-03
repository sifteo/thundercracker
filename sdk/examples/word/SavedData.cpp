#include "SavedData.h"
#include <sifteo.h>
#include "EventData.h"
#include "EventID.h"
#include <cstdlib>
#include "GameStateMachine.h"

unsigned SavedData::sHighScores[4];

SavedData::SavedData()
{
    for (unsigned i = 0; i < arraysize(sHighScores); ++i)
    {
        sHighScores[i] = 0;
    }
}

static int compare_ints(const void* a, const void* b)   // comparison function
{
    int* arg1 = (int*) a;
    int* arg2 = (int*) b;
    if( *arg1 < *arg2 ) return -1;
    else if( *arg1 == *arg2 ) return 0;
    else return 1;
}

void SavedData::sOnEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_EndRound:
        {
            unsigned score = GameStateMachine::GetScore();
            for (unsigned i = 0; i < arraysize(sHighScores); ++i)
            {
                if (score > sHighScores[i])
                {
                    sHighScores[i] = score;
                    qsort(sHighScores,
                          arraysize(sHighScores),
                          sizeof(sHighScores[0]),
                          compare_ints);
                    break;

                }
            }
        }
        break;

    default:
        break;
    }
}
