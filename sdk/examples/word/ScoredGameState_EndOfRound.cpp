#include "ScoredGameState_EndOfRound.h"
#include "ScoredGameState.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include "WordGame.h"
#include "assets.gen.h"
#include "CubeState.h"

unsigned ScoredGameState_EndOfRound::update(float dt, float stateTime)
{
    return GameStateIndex_EndOfRoundScored;
}

unsigned ScoredGameState_EndOfRound::onEvent(unsigned eventID, const EventData& data)
{

    return GameStateIndex_EndOfRoundScored;
}
