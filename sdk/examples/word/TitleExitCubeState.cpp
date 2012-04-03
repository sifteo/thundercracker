#include "TitleExitCubeState.h"
#include <sifteo.h>
#include "EventID.h"
#include "EventData.h"
#include "assets.gen.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "SavedData.h"

unsigned TitleExitCubeState::onEvent(unsigned eventID, const EventData& data)
{

    return getStateMachine().getCurrentStateIndex();
}

unsigned TitleExitCubeState::update(float dt, float stateTime)
{
    return 0;
}

void TitleExitCubeState::paint()
{

}
