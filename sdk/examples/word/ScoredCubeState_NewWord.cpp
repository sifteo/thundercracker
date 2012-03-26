#include <sifteo.h>
#include "EventID.h"
#include "EventData.h"
#include "assets.gen.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "ScoredCubeState_NewWord.h"
#include "WordGame.h"

using namespace Sifteo;

ScoredCubeState_NewWord::ScoredCubeState_NewWord()
{
}

unsigned ScoredCubeState_NewWord::onEvent(unsigned eventID, const EventData& data)
{
#if (0)
    switch (eventID)
    {
    case EventID_NewWordFound:
        if (getStateMachine().isConnectedToCubeOnSide(data.mWordFound.mCubeIDStart))
        {
            getStateMachine().resetStateTime();
        }
        // fall through
    case EventID_EnterState:
        {
            Cube& c = getStateMachine().getCube();
            mImageIndex = ImageIndex_ConnectedWord;
            if (c.physicalNeighborAt(SIDE_LEFT) == CUBE_ID_UNDEFINED &&
                c.physicalNeighborAt(SIDE_RIGHT) != CUBE_ID_UNDEFINED)
            {
                mImageIndex = ImageIndex_ConnectedLeftWord;
            }
            else if (c.physicalNeighborAt(SIDE_LEFT) != CUBE_ID_UNDEFINED &&
                     c.physicalNeighborAt(SIDE_RIGHT) == CUBE_ID_UNDEFINED)
            {
                mImageIndex = ImageIndex_ConnectedRightWord;
            }
        }
        WordGame::instance()->setNeedsPaintSync();
        // fall through

    case EventID_Paint:
    case EventID_ClockTick:
        paint();
        break;

    case EventID_AddNeighbor:
    case EventID_RemoveNeighbor:
    case EventID_LetterOrderChange:
        paint();
        break;

        /* TODO remove dead code
    case EventID_WordBroken:
        if (!getStateMachine().canBeginWord() &&
            getStateMachine().isConnectedToCubeOnSide(data.mWordBroken.mCubeIDStart))
        {
            return CubeStateIndex_NotWordScored;
        }
        break;


    case EventID_OldWordFound:
        if (!getStateMachine().canBeginWord() &&
             getStateMachine().isConnectedToCubeOnSide(data.mWordFound.mCubeIDStart))
        {
            return CubeStateIndex_OldWordScored;
        }
        break;
*/
    case EventID_GameStateChanged:
        switch (data.mGameStateChanged.mNewStateIndex)
        {
        case GameStateIndex_EndOfRoundScored:
            return CubeStateIndex_EndOfRoundScored;

        case GameStateIndex_ShuffleScored:
            return CubeStateIndex_ShuffleScored;
        }
        break;
    }
#endif
    return getStateMachine().getCurrentStateIndex();
}

unsigned ScoredCubeState_NewWord::update(float dt, float stateTime)
{
    return CubeState::update(dt, stateTime);
}

void ScoredCubeState_NewWord::paint()
{
}
