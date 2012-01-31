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
    case EventID_Paint:
    case EventID_ClockTick:
        paint();
        break;

    case EventID_AddNeighbor:
    case EventID_RemoveNeighbor:
    case EventID_LetterOrderChange:
        {
            bool isOldWord = false;
            if (getStateMachine().canBeginWord())
            {
                char wordBuffer[MAX_LETTERS_PER_WORD + 1];
                if (getStateMachine().beginsWord(isOldWord, wordBuffer))
                {
                    EventData data;
                    data.mWordFound.mCubeIDStart = getStateMachine().getCube().id();
                    data.mWordFound.mWord = wordBuffer;
                    if (isOldWord)
                    {
                        GameStateMachine::sOnEvent(EventID_OldWordFound, data);
                        return CubeStateIndex_OldWordScored;
                    }
                    else
                    {
                        GameStateMachine::sOnEvent(EventID_NewWordFound, data);
                        return CubeStateIndex_NewWordScored;
                    }
                }
                else
                {
                    EventData data;
                    data.mWordBroken.mCubeIDStart = getStateMachine().getCube().id();
                    GameStateMachine::sOnEvent(EventID_WordBroken, data);

                    return CubeStateIndex_NotWordScored;
                }
            }
            else if (getStateMachine().hasNoNeighbors())
            {
                return CubeStateIndex_NotWordScored;
            }

            paint();
        }
        break;

    case EventID_WordBroken:
        if (!getStateMachine().canBeginWord() &&
            getStateMachine().isConnectedToCubeOnSide(data.mWordBroken.mCubeIDStart))
        {
            return CubeStateIndex_NotWordScored;
        }
        break;

    case EventID_NewWordFound:
        if (!getStateMachine().canBeginWord() &&
             getStateMachine().isConnectedToCubeOnSide(data.mWordFound.mCubeIDStart))
        {
            return CubeStateIndex_NewWordScored;
        }
        break;

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
    return getStateMachine().getCurrentStateIndex();
}

unsigned ScoredCubeState_OldWord::update(float dt, float stateTime)
{
    CubeState::update(dt, stateTime);
    return getStateMachine().getCurrentStateIndex();
}

void ScoredCubeState_OldWord::paint()
{
    Cube& c = getStateMachine().getCube();
    VidMode_BG0_SPR_BG1 vid(c.vbuf);
    vid.init();

    switch (GameStateMachine::getCurrentMaxLettersPerCube())
    {
    case 2:
        paintLetters(vid, Font2Letter, true);
        break;

    default:
        paintLetters(vid, Font1Letter, true);
        break;
    }

    ImageIndex ii = ImageIndex_Connected;
    if (c.physicalNeighborAt(SIDE_LEFT) == CUBE_ID_UNDEFINED &&
        c.physicalNeighborAt(SIDE_RIGHT) != CUBE_ID_UNDEFINED)
    {
        ii = ImageIndex_ConnectedLeft;
    }
    else if (c.physicalNeighborAt(SIDE_LEFT) != CUBE_ID_UNDEFINED &&
             c.physicalNeighborAt(SIDE_RIGHT) == CUBE_ID_UNDEFINED)
    {
        ii = ImageIndex_ConnectedRight;
    }
    paintTeeth(vid, ii, true, false, true, false);
}
