#include "StateMachine.h"
#include "State.h"
#include <sifteo.h>
#include "EventID.h"

StateMachine::StateMachine(unsigned startStateIndex) :
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
            state->onEvent(EventID_EnterState);
            mUnhandledOnEnter = false;
        }
        unsigned newStateIndex = state->update(dt, mStateTime);
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

void StateMachine::onEvent(unsigned eventID)
{
    State* state = getState(mStateIndex);
    if (state != 0)
    {
        unsigned newStateIndex = state->onEvent(eventID);
        if (newStateIndex != mStateIndex)
        {
            setState(newStateIndex, state);
        }
    }
}

void StateMachine::setState(unsigned newStateIndex, State* oldState)
{
    ASSERT(newStateIndex < getNumStates());
    oldState->onEvent(EventID_ExitState);
    mStateIndex = newStateIndex;
    mStateTime = .0f;
    State* newState = getState(mStateIndex);
    newState->onEvent(EventID_EnterState);
}
