#include <sifteo.h>
#include "EventID.h"
#include "EventData.h"
#include "assets.gen.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"

#include "ScoredCubeState_OldWord.h"

ScoredCubeState_OldWord::ScoredCubeState_OldWord()
{
}

unsigned ScoredCubeState_OldWord::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    // TODO debug: case EventID_Paint:
    case EventID_EnterState:
        paint();
        if (getStateMachine().canBeginWord())
        {
            GameStateMachine::sOnEvent(EventID_OldWordFound, EventData());
        }
        break;

    case EventID_AddNeighbor:
    case EventID_RemoveNeighbor:
        {
            bool isOldWord = false;
            if (getStateMachine().canBeginWord())
            {
                if (getStateMachine().beginsWord(isOldWord))
                {
                    if (isOldWord)
                    {
                        return ScoredCubeSubstate_OldWord;
                    }
                }
                else
                {
                    return ScoredCubeSubstate_NotWord;
                }
            }
            else
            {
                return ScoredCubeSubstate_NotWord;
            }
            paint();
        }
        break;

    case EventID_WordBroken:
        if (!getStateMachine().canBeginWord() && !getStateMachine().isInWord())
        {
            return ScoredCubeSubstate_NotWord;
        }
        break;

    }
    return getStateMachine().getCurrentStateIndex();
}

unsigned ScoredCubeState_OldWord::update(float dt, float stateTime)
{
    return getStateMachine().getCurrentStateIndex();
}

void ScoredCubeState_OldWord::paint()
{
    Cube& c = getStateMachine().getCube();
    // FIXME vertical words
    const Sifteo::AssetImage& bg =
        (c.physicalNeighborAt(SIDE_LEFT) != CUBE_ID_UNDEFINED ||
         c.physicalNeighborAt(SIDE_RIGHT) != CUBE_ID_UNDEFINED) ?
            BGOldWordConnectedMiddle :
            BGOldWordConnectedLeft;
    VidMode_BG0 vid(c.vbuf);
    vid.init();
    vid.BG0_drawAsset(Vec2(0,0), bg);
    vid.BG0_text(Vec2(8,8), Font, getStateMachine().getLetters());
}
