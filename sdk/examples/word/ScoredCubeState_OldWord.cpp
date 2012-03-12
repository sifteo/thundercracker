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
#if (0)
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
                EventData wordFoundData;
                if (getStateMachine().beginsWord(isOldWord, wordBuffer, wordFoundData.mWordFound.mBonus))
                {
                    wordFoundData.mWordFound.mCubeIDStart = getStateMachine().getCube().id();
                    wordFoundData.mWordFound.mWord = wordBuffer;
                    if (isOldWord)
                    {
                        GameStateMachine::sOnEvent(EventID_OldWordFound, wordFoundData);
                        return CubeStateIndex_OldWordScored;
                    }
                    else
                    {
                        GameStateMachine::sOnEvent(EventID_NewWordFound, wordFoundData);
                        return CubeStateIndex_NewWordScored;
                    }
                }
                else
                {
                    EventData wordBrokenData;
                    wordBrokenData.mWordBroken.mCubeIDStart = getStateMachine().getCube().id();
                    GameStateMachine::sOnEvent(EventID_WordBroken, wordBrokenData);

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
#endif
    return getStateMachine().getCurrentStateIndex();
}

unsigned ScoredCubeState_OldWord::update(float dt, float stateTime)
{
    CubeState::update(dt, stateTime);
    return getStateMachine().getCurrentStateIndex();
}

void ScoredCubeState_OldWord::paint()
{

}
