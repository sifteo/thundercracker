#include "ScoredGameState_Shuffle.h"
#include "ScoredGameState.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include "WordGame.h"
#include "assets.gen.h"
#include "CubeState.h"

unsigned ScoredGameState_Shuffle::update(float dt, float stateTime)
{
return 0;
}

unsigned ScoredGameState_Shuffle::onEvent(unsigned eventID, const EventData& data)
{

    return GameStateIndex_ShuffleScored;
}
