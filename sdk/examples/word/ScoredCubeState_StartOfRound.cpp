#include <sifteo.h>
#include "EventID.h"
#include "EventData.h"
#include "assets.gen.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "Dictionary.h"
#include "ScoredCubeState_StartOfRound.h"

using namespace Sifteo;

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

    case EventID_GameStateChanged:
        switch (data.mGameStateChanged.mNewStateIndex)
        {
        case GameStateIndex_PlayScored:
            return CubeStateIndex_NotWordScored;
        }
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
#if (0)
    Cube& c = getStateMachine().getCube();
    VidMode_BG0_SPR_BG1 vid(c.vbuf);
    vid.init();
    // intro anim
    vid.BG0_drawAsset(vec(0, 0), TileBG);
    paintBorder(vid, ImageIndex_Teeth, true, false);
#endif
}
