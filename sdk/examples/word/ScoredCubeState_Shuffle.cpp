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
    switch (eventID)
    {
    case EventID_EnterState:
    case EventID_Paint:
        paint();
        break;
    }
    return getStateMachine().getCurrentStateIndex();
}

unsigned ScoredCubeState_Shuffle::update(float dt, float stateTime)
{
    return stateTime <= TEETH_ANIM_LENGTH * 2.f ?
                CubeStateIndex_ShuffleScored: CubeStateIndex_NotWordScored;
}

void ScoredCubeState_Shuffle::paint()
{
    Cube& c = getStateMachine().getCube();
    VidMode_BG0_SPR_BG1 vid(c.vbuf);
    vid.init();
    WordGame::hideSprites(vid);
    if (GameStateMachine::getTime() <= TEETH_ANIM_LENGTH)
    {
        // teeth closing animation
        paintLetters(vid, Font1Letter);
        paintTeeth(vid, ImageIndex_Teeth, true, true);
        DEBUG_LOG(("shuffle: [c: %d] teeth closing %f\n", c.id(), GameStateMachine::getTime()));
    }
    else
    {
        // teeth opening animation
        /*if (GameStateMachine::getTime() <= TEETH_ANIM_LENGTH - 0.2f)
        {
            paintLetters(vid, Font1Letter);
        }
        else*/
        {
            // no letters during blip
            vid.BG0_drawAsset(Vec2(0, 0), LetterBG);
        }
        paintTeeth(vid, ImageIndex_Teeth, true, false, false, false, TEETH_ANIM_LENGTH);
        DEBUG_LOG(("shuffle: [c: %d] teeth opening %f\n", c.id(), GameStateMachine::getTime()));
    }
}
