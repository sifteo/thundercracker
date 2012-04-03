#include "TitleCubeState.h"
#include <sifteo.h>
#include "EventID.h"
#include "EventData.h"
#include "assets.gen.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "SavedData.h"
#include "WordGame.h"
#include "config.h"


unsigned TitleCubeState::onEvent(unsigned eventID, const EventData& data)
{

    return getStateMachine().getCurrentStateIndex();
}

unsigned TitleCubeState::update(float dt, float stateTime)
{
    return getStateMachine().getCurrentStateIndex();
}

void TitleCubeState::paint()
{
}
