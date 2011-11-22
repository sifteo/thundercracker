#include "StateMachine.h"
#include "State.h"
#include <sifteo.h>

StateMachine::StateMachine(int startStateIndex) :
    mStateIndex(startStateIndex), mStateTime(.0f)
{
}

void StateMachine::Update(float dt)
{
    State* state = GetState(mStateIndex);
    if (state != 0)
    {
        int newStateIndex = state->Update(dt, mStateTime);
        if (newStateIndex != mStateIndex)
        {
            SetState(newStateIndex, state);
        }
        else
        {
            mStateTime += dt;
        }
    }
}

void StateMachine::OnEvent(int eventID)
{
    State* state = GetState(mStateIndex);
    if (state != 0)
    {
        int newStateIndex = state->OnEvent(eventID);
        if (newStateIndex != mStateIndex)
        {
            SetState(newStateIndex, state);
        }
    }
}

void StateMachine::SetState(int newStateIndex, State* oldState)
{
    ASSERT(newStateIndex < GetNumStates());
    oldState->OnExit();
    mStateIndex = newStateIndex;
    mStateTime = .0f;
    State* newState = GetState(mStateIndex);
    newState->OnEnter();
}
