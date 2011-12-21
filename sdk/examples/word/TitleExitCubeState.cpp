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
    switch (eventID)
    {
    // TODO debug: case EventID_Paint:
    case EventID_EnterState:
    case EventID_Paint:
        paint();
        break;
    }
    return getStateMachine().getCurrentStateIndex();
}

unsigned TitleExitCubeState::update(float dt, float stateTime)
{
    return getStateMachine().getTime() > TEETH_ANIM_LENGTH ? CubeStateIndex_NotWordScored : CubeStateIndex_TitleExit;
}

void TitleExitCubeState::paint()
{
    Cube& c = getStateMachine().getCube();
    // FIXME vertical words
/*    const Sifteo::AssetImage& bg =
        (c.physicalNeighborAt(SIDE_LEFT) != CUBE_ID_UNDEFINED ||
         c.physicalNeighborAt(SIDE_RIGHT) != CUBE_ID_UNDEFINED) ?
            BGNewWordConnectedMiddle :
            BGNewWordConnectedLeft;
            */
    VidMode_BG0_SPR_BG1 vid(c.vbuf);
    vid.init();
    paintTeeth(vid, TeethLoopWord, true, false);
}
