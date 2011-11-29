#include <sifteo.h>
#include "EventID.h"
#include "EventData.h"
#include "assets.gen.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "ScoredCubeState_NewWord.h"

ScoredCubeState_NewWord::ScoredCubeState_NewWord()
{
}

unsigned ScoredCubeState_NewWord::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    // TODO debug: case EventID_Paint:
    case EventID_EnterState:
        paint();

        if (getStateMachine().canBeginWord())
        {
            GameStateMachine::sOnEvent(EventID_NewWordFound, EventData());
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
            else if (!getStateMachine().isInWord())
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

unsigned ScoredCubeState_NewWord::update(float dt, float stateTime)
{
    return getStateMachine().getCurrentStateIndex();
}

void ScoredCubeState_NewWord::paint()
{
    Cube& c = getStateMachine().getCube();
    // FIXME vertical words
    const Sifteo::AssetImage& bg =
        (c.physicalNeighborAt(SIDE_LEFT) != CUBE_ID_UNDEFINED ||
         c.physicalNeighborAt(SIDE_RIGHT) != CUBE_ID_UNDEFINED) ?
            BGNewWordConnectedMiddle :
            BGNewWordConnectedLeft;
    VidMode_BG0 vid(c.vbuf);
    vid.init();
    vid.BG0_drawAsset(Vec2(0,0), bg);
    vid.BG0_text(Vec2(8,8), Font, getStateMachine().getLetters());
}
