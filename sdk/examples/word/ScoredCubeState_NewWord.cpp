#include <sifteo.h>
#include "EventID.h"
#include "EventData.h"
#include "assets.gen.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "ScoredCubeState_NewWord.h"
#include <string.h>
#include "WordGame.h"

ScoredCubeState_NewWord::ScoredCubeState_NewWord()
{
}

unsigned ScoredCubeState_NewWord::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_NewWordFound:
        if (getStateMachine().isConnectedToCubeOnSide(data.mWordFound.mCubeIDStart))
        {
            getStateMachine().resetStateTime();
        }
        // fall through
    case EventID_EnterState:
        WordGame::instance()->setNeedsPaintSync();
        // fall through

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


    case EventID_OldWordFound:
        if (!getStateMachine().canBeginWord() &&
             getStateMachine().isConnectedToCubeOnSide(data.mWordFound.mCubeIDStart))
        {
            return CubeStateIndex_OldWordScored;
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

unsigned ScoredCubeState_NewWord::update(float dt, float stateTime)
{
    CubeState::update(dt, stateTime);
    if (getStateMachine().getTime() <= TEETH_ANIM_LENGTH)
    {
        return CubeStateIndex_NewWordScored;
    }
    else
    {
        return CubeStateIndex_OldWordScored;
    }
}

void ScoredCubeState_NewWord::paint()
{
    Cube& c = getStateMachine().getCube();
    VidMode_BG0_SPR_BG1 vid(c.vbuf);
    vid.init();
//    paintLetters(vid, Font1Letter, true);
    vid.BG0_drawAsset(Vec2(0,0), ScreenOff);
    ImageIndex ii = ImageIndex_ConnectedWord;
    if (c.physicalNeighborAt(SIDE_LEFT) == CUBE_ID_UNDEFINED &&
        c.physicalNeighborAt(SIDE_RIGHT) != CUBE_ID_UNDEFINED)
    {
        ii = ImageIndex_ConnectedLeftWord;
    }
    else if (c.physicalNeighborAt(SIDE_LEFT) != CUBE_ID_UNDEFINED &&
             c.physicalNeighborAt(SIDE_RIGHT) == CUBE_ID_UNDEFINED)
    {
        ii = ImageIndex_ConnectedRightWord;
    }

    paintTeeth(vid, ii, true, false, false, false);
    vid.BG0_setPanning(Vec2(0.f, 0.f));
}
