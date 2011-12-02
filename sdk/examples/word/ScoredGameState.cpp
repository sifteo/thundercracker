#include "ScoredGameState.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include <string.h>

unsigned ScoredGameState::update(float dt, float stateTime)
{
    mAnagramCooldown -= dt;
    mAnagramCooldown = MAX(.0f, mAnagramCooldown);
    mTimeLeft -= dt;
    return (mTimeLeft <= .0f) ? ScoredGameStateIndex_EndOfRound : ScoredGameStateIndex_Play;
}

unsigned ScoredGameState::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_EnterState:
        mTimeLeft = ROUND_TIME;
        mAnagramCooldown = .0f;
        mScore = 0;
        // fall through
    case EventID_Input:
        if (mAnagramCooldown <= .0f)
        {
            EventData data;
            data.mNewAnagram.mWord = Dictionary::pickWord(MAX_CUBES);
            unsigned wordLen = strlen(data.mNewAnagram.mWord);
            unsigned numCubes = GameStateMachine::GetNumCubes();
            if ((wordLen % numCubes) == 0)
            {
                // all cubes have word fragments of the same length
                data.mNewAnagram.mOffLengthIndex = -1;
            }
            else
            {
                // pick a cube index randomly to have the off length
                // word fragment
                //data.mNewAnagram.mOddIndex = ;
            }
            GameStateMachine::sOnEvent(EventID_NewAnagram, data);
            mAnagramCooldown = ANAGRAM_COOLDOWN;
        }
        break;

    default:
        break;
    }
    return ScoredGameStateIndex_Play;
}


