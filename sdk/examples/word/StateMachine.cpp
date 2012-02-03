#include "StateMachine.h"
#include "State.h"
#include <sifteo.h>
#include "EventID.h"
#include "EventData.h"

StateMachine::StateMachine(unsigned startStateIndex) :
    mStateTime(.0f), mStateIndex(startStateIndex), mUnhandledOnEnter(true)
{
}

void StateMachine::update(float dt)
{
    State& state = getState(mStateIndex);
    if (mUnhandledOnEnter)
    {
        EventData data;
        data.mEnterState.mFirst = true;
        data.mEnterState.mPreviousStateIndex = -1;
        state.onEvent(EventID_EnterState, data);
        mUnhandledOnEnter = false;
    }
    unsigned newStateIndex = state.update(dt, mStateTime);
    if (newStateIndex != mStateIndex)
    {
        setState(newStateIndex, state);
    }
    else
    {
        mStateTime += dt;
    }
}

void StateMachine::onEvent(unsigned eventID, const EventData& data)
{
    State& state = getState(mStateIndex);
    unsigned newStateIndex = state.onEvent(eventID, data);
    if (newStateIndex != mStateIndex)
    {
        setState(newStateIndex, state);
    }
}

void StateMachine::setState(unsigned newStateIndex, State& oldState)
{
    ASSERT(newStateIndex < getNumStates());
    unsigned newAttemptedStateIndex =
            oldState.onEvent(EventID_ExitState, EventData());
    ASSERT(newAttemptedStateIndex == newStateIndex ||
           newAttemptedStateIndex == mStateIndex); // can't change state on this event
    EventData data;
    data.mEnterState.mFirst = false;
    data.mEnterState.mPreviousStateIndex = mStateIndex;
    mStateIndex = newStateIndex;
    mStateTime = .0f;
    State& newState = getState(mStateIndex);    
    newAttemptedStateIndex =
        newState.onEvent(EventID_EnterState, data);
    ASSERT(newAttemptedStateIndex == newStateIndex); // can't change state on this event
}
