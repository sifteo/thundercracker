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
