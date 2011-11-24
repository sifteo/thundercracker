#include "ScoredGameState.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"

unsigned ScoredGameState::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_EnterState:
        {
            EventData data;
            data.mNewAnagram = Dictionary::pickWord(MAX_CUBES);
            GameStateMachine::sOnEvent(EventID_NewAnagram, data);
        }
        break;

    default:
        break;
    }
    return 0;
}


