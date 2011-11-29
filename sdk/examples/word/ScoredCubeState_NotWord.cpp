#include <sifteo.h>
#include "EventID.h"
#include "EventData.h"
#include "assets.gen.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "Dictionary.h"
#include "ScoredCubeState_NotWord.h"

ScoredCubeState_NotWord::ScoredCubeState_NotWord()
{
}

unsigned ScoredCubeState_NotWord::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    // TODO debug: case EventID_Paint:
    case EventID_EnterState:
        if (!data.mEnterState.mFirst)
        {
            GameStateMachine::sOnEvent(EventID_WordBroken, EventData());
        }
        // fall through
    case EventID_NewAnagram:        
        paint();
        break;

    case EventID_AddNeighbor:
    case EventID_RemoveNeighbor:
        {
            bool isOldWord = false;
            if (getStateMachine().beginsWord(isOldWord))
            {
                return (isOldWord) ?
                            ScoredCubeSubstate_OldWord :
                            ScoredCubeSubstate_NewWord;
            }
            paint();
        }
        break;

    case EventID_NewWordFound:
        if (!getStateMachine().canBeginWord() && getStateMachine().isInWord())
        {
            return ScoredCubeSubstate_NewWord;
        }
        break;

    case EventID_OldWordFound:
        if (!getStateMachine().canBeginWord() && getStateMachine().isInWord())
        {
            return ScoredCubeSubstate_OldWord;
        }
        break;

    }
    return getStateMachine().getCurrentStateIndex();
}

unsigned ScoredCubeState_NotWord::update(float dt, float stateTime)
{
    return getStateMachine().getCurrentStateIndex();
}

void ScoredCubeState_NotWord::paint()
{
    Cube& c = getStateMachine().getCube();
    // FIXME vertical words
    const Sifteo::AssetImage& bg =
        (c.physicalNeighborAt(SIDE_LEFT) != CUBE_ID_UNDEFINED ||
         c.physicalNeighborAt(SIDE_RIGHT) != CUBE_ID_UNDEFINED) ?
            BGNotWordConnected :
            BGNotWordNotConnected;
    VidMode_BG0 vid(c.vbuf);
    vid.init();
    vid.BG0_drawAsset(Vec2(0,0), bg);
    vid.BG0_text(Vec2(8,8), Font, getStateMachine().getLetters());
}
