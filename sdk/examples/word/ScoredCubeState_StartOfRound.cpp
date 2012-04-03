#include <sifteo.h>
#include "EventID.h"
#include "EventData.h"
#include "assets.gen.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "Dictionary.h"
#include "ScoredCubeState_StartOfRound.h"

ScoredCubeState_StartOfRound::ScoredCubeState_StartOfRound()
{
}

unsigned ScoredCubeState_StartOfRound::onEvent(unsigned eventID, const EventData& data)
{
    return getStateMachine().getCurrentStateIndex();
}

unsigned ScoredCubeState_StartOfRound::update(float dt, float stateTime)
{
    return getStateMachine().getCurrentStateIndex();
}

void ScoredCubeState_StartOfRound::paint()
{

}
