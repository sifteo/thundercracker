#include "GameStateMachine.h"
#include "Dictionary.h"

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
    Dictionary::sOnEvent(eventID, data);
}

void GameStateMachine::sOnEvent(unsigned eventID, const EventData& data)
{
    if (sInstance != 0)
    {
        sInstance->onEvent(eventID, data);
    }
}


CubeStateMachine* GameStateMachine::findCSMFromID(Cube::ID cubeID)
{
    if (cubeID == CUBE_ID_UNDEFINED)
    {
        return 0;
    }

    if (sInstance)
    {
        for (unsigned i = 0; i < arraysize(sInstance->mCubeStateMachines); ++i)
        {
            if (sInstance->mCubeStateMachines[i].getCube().id() == cubeID)
            {
                return &sInstance->mCubeStateMachines[i];
            }
        }
    }
    ASSERT(0);
    return 0;
}

State& GameStateMachine::getState(unsigned index)
{
    // TODO simply the state machine code
    ASSERT(index < ScoredGameStateIndex_NumStates);
    switch (index)
    {
    default:
        return mScoredState;
    case ScoredGameStateIndex_EndOfRound:
        return mScoredEndOfRoundState;
    }
}
