#include "ScoredGameState.h"
#include "EventID.h"
#include "Dictionary.h"

unsigned ScoredGameState::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_EnterState:
        {
            const char* word = Dictionary::pickWord(6);
        }
        break;

    default:
        break;
    }
    return 0;
}


