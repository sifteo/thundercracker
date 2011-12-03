#include "ScoredGameState_EndOfRound.h"
#include "ScoredGameState.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include <string.h>

unsigned ScoredGameState_EndOfRound::update(float dt, float stateTime)
{
    return ScoredGameStateIndex_EndOfRound;
}

unsigned ScoredGameState_EndOfRound::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_EnterState:
        GameStateMachine::sOnEvent(EventID_EndRound, EventData());
        break;

    case EventID_Input:
        return ScoredGameStateIndex_Play;

    default:
        break;
    }
    return ScoredGameStateIndex_EndOfRound;
}
