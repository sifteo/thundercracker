#include "ScoredGameState_Shuffle.h"
#include "ScoredGameState.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include <string.h>
#include "WordGame.h"
#include "audio.gen.h"

unsigned ScoredGameState_Shuffle::update(float dt, float stateTime)
{
    if (stateTime > TEETH_ANIM_LENGTH && stateTime - dt <= TEETH_ANIM_LENGTH)
    {
        ScoredGameState::createNewAnagram();
    }

    return (stateTime > TEETH_ANIM_LENGTH * 2.f) ? GameStateIndex_PlayScored : GameStateIndex_ShuffleScored;
}

unsigned ScoredGameState_Shuffle::onEvent(unsigned eventID, const EventData& data)
{
    ScoredGameState::onAudioEvent(eventID, data);
    switch (eventID)
    {
    case EventID_EnterState:
        break;

    default:
        break;
    }
    return GameStateIndex_ShuffleScored;
}
