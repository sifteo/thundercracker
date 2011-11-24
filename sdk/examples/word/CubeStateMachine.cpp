#include "CubeStateMachine.h"

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
