#include "SavedData.h"
#include <sifteo.h>
#include "EventData.h"
#include "EventID.h"
#include "GameStateMachine.h"
#include "assets.gen.h"
#include "WordGame.h"
#include "Dictionary.h"

using namespace Sifteo;

/*
 * XXX: Only used for qsort() currently. We should think about what kind of low-level VM
 *      primitives the sort should be based on (with regard to ABI, as well as cache
 *      behavior) and design it with a proper syscall interface. But for now, we're leaking
 *      some libc code into the game :(
 */
// #include <stdlib.h>
#include "qsort.h"
SavedData::SavedData()
{
    for (unsigned i = 0; i < arraysize(mHighScores); ++i)
    {
        for (unsigned j = 0; j < arraysize(mHighScores[0]); ++j)
        {
            mHighScores[i][j] = 0;
        }
    }

    for (unsigned j = 0; j < arraysize(mCompletedPuzzles); ++j)
    {
        mCompletedPuzzles[j] = 0;
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

void SavedData::OnEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_PuzzleSolved:
        {
            mLastSolvedPuzzle = Dictionary::getPuzzleIndex();
            LOG("puzzle solved event, %d\n", Dictionary::getPuzzleIndex());
            unsigned byteIndex = mLastSolvedPuzzle / 8;
            unsigned bitIndex = mLastSolvedPuzzle % 8;
            ASSERT(byteIndex < arraysize(mCompletedPuzzles));
            mCompletedPuzzles[byteIndex] |= (1 << bitIndex);
        }
        break;

    case EventID_GameStateChanged:
        if (data.mGameStateChanged.mNewStateIndex == GameStateIndex_EndOfRoundScored)
        {
            unsigned score = GameStateMachine::getScore();
            for (unsigned i = 0; i < arraysize(mHighScores[0]); ++i)
            {
                if (score > mHighScores[NUM_CUBES][i])
                {
                    mHighScores[NUM_CUBES][i] = score;
                    qsort(mHighScores[NUM_CUBES],
                          arraysize(mHighScores[0]),
                          sizeof(mHighScores[0][0]),
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
            if (score == mHighScores[NUM_CUBES][arraysize(mHighScores[0]) - 1])
            {
                endType = EndingType_TopHighScore;
            }
            else if (score > 0 && score >= mHighScores[0][0])
            {
                endType = EndingType_HighScore;
            }
            WordGame::playAudio(*EndingJingles[(unsigned)endType],
                                AudioChannelIndex_Time,
                                AudioChannel::ONCE,
                                AudioPriority_High);
        }
        break;

    default:
        break;
    }
}

bool SavedData::isPuzzleSolved(unsigned index) const
{
    unsigned byteIndex = index / 8;
    unsigned bitIndex = index % 8;
    //LOG("isPuzzleSolved: %d?, %d\n", index, (mCompletedPuzzles[byteIndex] & (1 << bitIndex)) != 0);
    ASSERT(byteIndex < arraysize(mCompletedPuzzles));
    return (mCompletedPuzzles[byteIndex] & (1 << bitIndex)) != 0;
}
