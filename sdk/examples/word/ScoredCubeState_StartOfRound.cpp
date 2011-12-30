#include <sifteo.h>
#include "EventID.h"
#include "EventData.h"
#include "assets.gen.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "Dictionary.h"
#include "ScoredCubeState_StartOfRound.h"
#include <string.h>

ScoredCubeState_StartOfRound::ScoredCubeState_StartOfRound()
{
}

unsigned ScoredCubeState_StartOfRound::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_EnterState:
    case EventID_NewAnagram:
    case EventID_Paint:
    case EventID_ClockTick:
        paint();
        break;

    }
    return getStateMachine().getCurrentStateIndex();
}

unsigned ScoredCubeState_StartOfRound::update(float dt, float stateTime)
{
    return getStateMachine().getCurrentStateIndex();
}

void ScoredCubeState_StartOfRound::paint()
{
    Cube& c = getStateMachine().getCube();
    // FIXME vertical words
    bool neighbored =
            (c.physicalNeighborAt(SIDE_LEFT) != CUBE_ID_UNDEFINED ||
            c.physicalNeighborAt(SIDE_RIGHT) != CUBE_ID_UNDEFINED);
    VidMode_BG0_SPR_BG1 vid(c.vbuf);
    vid.init();
    // intro anim
    vid.BG0_drawAsset(Vec2(0, 0), LetterBG);
    paintTeeth(vid, ImageIndex_Teeth, true, false);
}
