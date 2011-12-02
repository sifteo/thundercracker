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
    case EventID_NewAnagram:        
        paint();
        break;

    case EventID_AddNeighbor:
    case EventID_RemoveNeighbor:
        {
            bool isOldWord = false;
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

                GameStateMachine::sOnEvent(EventID_NewWordFound, data);
                return ScoredCubeStateIndex_NewWord;
            }
            paint();
        }
        break;

    case EventID_NewWordFound:
        if (!getStateMachine().canBeginWord() &&
             getStateMachine().isConnectedToCubeOnSide(data.mWordFound.mCubeIDStart))
        {
            return ScoredCubeStateIndex_NewWord;
        }
        break;

    case EventID_OldWordFound:
        if (!getStateMachine().canBeginWord() &&
             getStateMachine().isConnectedToCubeOnSide(data.mWordFound.mCubeIDStart))
        {
            return ScoredCubeStateIndex_OldWord;
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
