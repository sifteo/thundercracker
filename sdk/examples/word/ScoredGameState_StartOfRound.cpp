#include "ScoredGameState_StartOfRound.h"
#include "ScoredGameState.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include "WordGame.h"
#include "assets.gen.h"
#include "CubeState.h"

unsigned ScoredGameState_StartOfRound::update(float dt, float stateTime)
{


    return GameStateIndex_StartOfRoundScored;
}

unsigned ScoredGameState_StartOfRound::onEvent(unsigned eventID, const EventData& data)
{
    return GameStateIndex_StartOfRoundScored;
}
