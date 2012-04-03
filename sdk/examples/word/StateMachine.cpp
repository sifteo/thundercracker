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
    if (mUnhandledOnEnter)
    {
        EventData data;
        data.mEnterState.mFirst = true;
        data.mEnterState.mPreviousStateIndex = -1;
        onEvent(EventID_EnterState, data);
        mUnhandledOnEnter = false;
    }
    EventData data;
    data.mUpdate.mDT = dt;
    unsigned newStateIndex = onEvent(EventID_Update, data);
    if (newStateIndex != mStateIndex)
    {
        setState(newStateIndex, mStateIndex);
    }
    else
    {
        mStateTime += dt;
    }
}

void StateMachine::setState(unsigned newStateIndex, unsigned oldStateIndex)
{
    ASSERT(newStateIndex < getNumStates());
    onEvent(EventID_ExitState, EventData());
    EventData data;
    data.mEnterState.mFirst = false;
    data.mEnterState.mPreviousStateIndex = mStateIndex;
    mStateIndex = newStateIndex;
    mStateTime = .0f;
    onEvent(EventID_EnterState, data);
}
