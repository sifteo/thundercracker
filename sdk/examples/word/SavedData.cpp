#include "SavedData.h"
#include <sifteo.h>
#include "EventData.h"
#include "EventID.h"
#include "GameStateMachine.h"
#include "assets.gen.h"
#include "WordGame.h"

using namespace Sifteo;

/*
 * XXX: Only used for qsort() currently. We should think about what kind of low-level VM
 *      primitives the sort should be based on (with regard to ABI, as well as cache
 *      behavior) and design it with a proper syscall interface. But for now, we're leaking
 *      some libc code into the game :(
 */
#include <stdlib.h>

unsigned SavedData::sHighScores[3];

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
    if ( *arg1 < *arg2 ) return -1;
    else if ( *arg1 == *arg2 ) return 0;
    else return 1;
}

void SavedData::sOnEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_GameStateChanged:
        if (data.mGameStateChanged.mNewStateIndex == GameStateIndex_EndOfRoundScored)
        {
            unsigned score = GameStateMachine::getScore();
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

            enum EndingType
            {
                EndingType_NoHighScore,
                EndingType_HighScore,
                EndingType_TopHighScore,

                NumEndingTypes
            };

            const AssetAudio* EndingJingles[NumEndingTypes] =
            {
                &timeup_01, &timeup_02, &timeup_03
            };

            EndingType endType = EndingType_NoHighScore;
            if (score == sHighScores[arraysize(sHighScores) - 1])
            {
                endType = EndingType_TopHighScore;
            }
            else if (score > 0 && score >= sHighScores[0])
            {
                endType = EndingType_HighScore;
            }
            WordGame::playAudio(*EndingJingles[(unsigned)endType],
                                AudioChannelIndex_Time,
                                LoopOnce,
                                AudioPriority_High);
        }
        break;

    default:
        break;
    }
}
