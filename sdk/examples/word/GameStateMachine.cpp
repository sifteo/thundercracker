#include "GameStateMachine.h"

GameStateMachine::GameStateMachine(Cube cubes[]) :
    StateMachine(0)
{
    for (unsigned i = 0; i < arraysize(mCubeStateMachines); ++i)
    {
        mCubeStateMachines[i].setCube(&cubes[i]);
    }
}
