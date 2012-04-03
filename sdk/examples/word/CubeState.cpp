#include <sifteo.h>
#include "CubeState.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "assets.gen.h"
#include "WordGame.h"
#include "TileTransparencyLookup.h"
#include "PartialAnimationData.h"
#include "config.h"
#include <string.h>
#include <stdio.h>

void CubeState::setStateMachine(CubeStateMachine& csm)
{
    mStateMachine = &csm;

    Cube& cube = csm.getCube();
    VidMode_BG0_SPR_BG1 vid(cube.vbuf);
    vid.init();
}

CubeStateMachine& CubeState::getStateMachine()
{
    ASSERT(mStateMachine != 0);
    return *mStateMachine;
}


unsigned CubeState::update(float dt, float stateTime)
{
    return getStateMachine().getCurrentStateIndex();
}


