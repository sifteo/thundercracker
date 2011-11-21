#include "StateMachine.h"
#include "State.h"
#include <sifteo.h>

StateMachine::StateMachine(int startStateIndex) :
    mNumStates(0), mStateIndex(startStateIndex), mStateTime(.0f)
{
    for (int i = 0; i < MAX_STATES; ++i)
    {
        mStates[i] = 0;
    }
}

void StateMachine::Update(float dt)
{
    State* state = mStates[mStateIndex];
    if (state != 0)
    {
        int newStateIndex = state->Update(dt, mStateTime);
        if (newStateIndex != mStateIndex)
        {
            ASSERT(newStateIndex < MAX_STATES);
            state->OnExit();
            mStateIndex = newStateIndex;
            mStates[mStateIndex]->OnEnter();
        }
        else
        {
            mStateTime += dt;
        }
    }
}

void StateMachine::AddState(State *s)
{
    ASSERT(mNumStates < MAX_STATES);
    mStates[mNumStates++] = s;
}
