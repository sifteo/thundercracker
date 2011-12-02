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
    case EventID_Paint:
        paint();
        break;

    case EventID_AddNeighbor:
    case EventID_RemoveNeighbor:
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
                        return ScoredCubeStateIndex_OldWord;
                    }
                    else
                    {
                        GameStateMachine::sOnEvent(EventID_NewWordFound, data);
                        return ScoredCubeStateIndex_NewWord;
                    }
                }
                else
                {
                    EventData data;
                    data.mWordBroken.mCubeIDStart = getStateMachine().getCube().id();
                    GameStateMachine::sOnEvent(EventID_WordBroken, data);
                    return ScoredCubeStateIndex_NotWord;
                }
            }
            else if (getStateMachine().hasNoNeighbors())
            {
                return ScoredCubeStateIndex_NotWord;
            }
            paint();
        }
        break;

    case EventID_WordBroken:
        if (!getStateMachine().canBeginWord() &&
            getStateMachine().isConnectedToCubeOnSide(data.mWordBroken.mCubeIDStart))
        {
            return ScoredCubeStateIndex_NotWord;
        }
        break;


    case EventID_OldWordFound:
        if (!getStateMachine().canBeginWord() &&
             getStateMachine().isConnectedToCubeOnSide(data.mWordFound.mCubeIDStart))
        {
            return ScoredCubeStateIndex_OldWord;
        }
        break;

    case EventID_EndRound:
        return ScoredCubeStateIndex_EndOfRound;
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
    vid.BG0_text(Vec2(6,3), Font, getStateMachine().getLetters());
    char string[5];
    sprintf(string, "%.4d", GameStateMachine::GetSecondsLeft());
#if DEBUG
    printf("%d %s\n", getStateMachine().getCube().id(), string);
#endif
    vid.BG0_text(Vec2(6,14), FontSmall, string);
}
