#include "ScoredCubeState.h"
#include "EventID.h"
#include "assets.gen.h"


ScoredCubeState::ScoredCubeState()
{
}

unsigned ScoredCubeState::onEvent(unsigned eventID, const EventData& data)
{
    ASSERT(mCube != 0);

    switch (eventID)
    {
    // TODO debug: case EventID_Paint:
    case EventID_EnterState:
        VidMode_BG0 vid(mCube->vbuf);
        vid.init();
        vid.BG0_drawAsset(Vec2(0,0), Background);
        vid.BG0_text(Vec2(8,8), Font, "hi");
        break;

    }
    return 0;
}
