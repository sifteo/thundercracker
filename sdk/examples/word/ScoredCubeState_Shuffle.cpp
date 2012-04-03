#include "ScoredCubeState_Shuffle.h"

#include <sifteo.h>
#include "EventID.h"
#include "EventData.h"
#include "assets.gen.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "ScoredCubeState_Shuffle.h"
#include "SavedData.h"
#include "WordGame.h"

unsigned ScoredCubeState_Shuffle::onEvent(unsigned eventID, const EventData& data)
{

    return getStateMachine().getCurrentStateIndex();
}

unsigned ScoredCubeState_Shuffle::update(float dt, float stateTime)
{
    return 0;
}

void ScoredCubeState_Shuffle::paint()
{}
