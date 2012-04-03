#include "ScoredCubeState_EndOfRound.h"

#include <sifteo.h>
#include "EventID.h"
#include "EventData.h"
#include "assets.gen.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "ScoredCubeState_EndOfRound.h"
#include "SavedData.h"
#include "WordGame.h"
#include "config.h"
#include <string.h>
#include <stdio.h>

const unsigned START_SCREEN_CUBE_INDEX = 1;
const unsigned SCORE_RHS_X = 9;
const unsigned SCORE_RHS_Y = 2;

unsigned ScoredCubeState_EndOfRound::onEvent(unsigned eventID, const EventData& data)
{

    return getStateMachine().getCurrentStateIndex();
}

unsigned ScoredCubeState_EndOfRound::update(float dt, float stateTime)
{
    return getStateMachine().getCurrentStateIndex();
}

void ScoredCubeState_EndOfRound::paint()
{
}
