#include "GameStateMachine.h"

GameStateMachine* GameStateMachine::sInstance = 0;

GameStateMachine::GameStateMachine(Cube cubes[]) :
    StateMachine(0)
{
    ASSERT(cubes != 0);
    sInstance = this;
    for (unsigned i = 0; i < arraysize(mCubeStateMachines); ++i)
    {
        mCubeStateMachines[i].setCube(cubes[i]);
    }
}

void GameStateMachine::update(float dt)
{
    StateMachine::update(dt);
    for (unsigned i = 0; i < arraysize(mCubeStateMachines); ++i)
    {
        mCubeStateMachines[i].update(dt);
    }
}

void GameStateMachine::onEvent(unsigned eventID, const EventData& data)
{
    StateMachine::onEvent(eventID, data);
    for (unsigned i = 0; i < arraysize(mCubeStateMachines); ++i)
    {
        mCubeStateMachines[i].onEvent(eventID, data);
    }
}

void GameStateMachine::sOnEvent(unsigned eventID, const EventData& data)
{
    ASSERT(sInstance != 0);
    sInstance->onEvent(eventID, data);
}
