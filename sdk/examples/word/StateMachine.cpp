#include "StateMachine.h"
#include "State.h"
#include <sifteo.h>

StateMachine::StateMachine(int startStateIndex) :
    mStateIndex(startStateIndex), mStateTime(.0f), mUnhandledOnEnter(true)
{
}

void StateMachine::update(float dt)
{
    State* state = getState(mStateIndex);
    if (state != 0)
    {
        if (mUnhandledOnEnter)
        {
            state->onEnter();
            mUnhandledOnEnter = false;
        }
        int newStateIndex = state->update(dt, mStateTime);
        if (newStateIndex != mStateIndex)
        {
            setState(newStateIndex, state);
        }
        else
        {
            mStateTime += dt;
        }
    }
}

void StateMachine::onEvent(int eventID)
{
    State* state = getState(mStateIndex);
    if (state != 0)
    {
        int newStateIndex = state->onEvent(eventID);
        if (newStateIndex != mStateIndex)
        {
            setState(newStateIndex, state);
        }
    }
}

void StateMachine::setState(unsigned newStateIndex, State* oldState)
{
    ASSERT(newStateIndex < getNumStates());
    oldState->onExit();
    mStateIndex = newStateIndex;
    mStateTime = .0f;
    State* newState = getState(mStateIndex);
    newState->onEnter();
}
