#include "CubeStateMachine.h"
#include "EventID.h"
#include "EventData.h"

void CubeStateMachine::setCube(Cube& cube)
{
    mCube = &cube;
    for (unsigned i = 0; i < getNumStates(); ++i)
    {
        CubeState& state = (CubeState&)getState(i);
        state.setStateMachine(*this);
    }
}

Cube& CubeStateMachine::getCube()
{
    ASSERT(mCube != 0);
    return *mCube;
}

void CubeStateMachine::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_NewAnagram:
        uint8_t cubeID = getCube().id();
        for (unsigned i = 0; i < arraysize(mLetters); ++i)
        {
            mLetters[i] = '\0';
        }
        for (unsigned i = 0; i < mNumLetters; ++i)
        {
            mLetters[i] = data.mNewAnagram.mWord[cubeID + i];
        }
        // TODO substrings of length 1 to 3
        break;
    }
    StateMachine::onEvent(eventID, data);
}


const char* CubeStateMachine::getLetters()
{
    ASSERT(mNumLetters > 0);
    return mLetters;
}

State& CubeStateMachine::getState(unsigned index)
{
    ASSERT(index < getNumStates());
    switch (index)
    {
    default:
    case ScoredCubeSubstate_NotWord:
        return mNotWordScoredState;

    case ScoredCubeSubstate_NewWord:
        return mNewWordScoredState;

    case ScoredCubeSubstate_OldWord:
        return mOldWordScoredState;
    }
}

unsigned CubeStateMachine::getNumStates() const
{
    return ScoredCubeSubstate_NumStates;
}
