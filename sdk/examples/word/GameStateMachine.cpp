#include "GameStateMachine.h"
#include "Dictionary.h"
#include "EventID.h"
#include "EventData.h"

GameStateMachine* GameStateMachine::sInstance = 0;

GameStateMachine::GameStateMachine(Cube cubes[]) :
    StateMachine(0), mAnagramCooldown(0.f), mTimeLeft(.0f), mScore(0)
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
    mAnagramCooldown -= dt;
    mAnagramCooldown = MAX(.0f, mAnagramCooldown);
    unsigned oldSecsLeft = GetSecondsLeft();
    mTimeLeft -= dt;
    mTimeLeft = MAX(.0f, mTimeLeft);

    if (oldSecsLeft != GetSecondsLeft())
    {
        // TODO dirty flags?
        onEvent(EventID_Paint, EventData());
    }

    StateMachine::update(dt);
    for (unsigned i = 0; i < arraysize(mCubeStateMachines); ++i)
    {
        mCubeStateMachines[i].update(dt);
    }
}

void GameStateMachine::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_NewRound:
        mTimeLeft = ROUND_TIME;
        mAnagramCooldown = .0f;
        mScore = 0;
        break;

    case EventID_NewAnagram:
        mAnagramCooldown = ANAGRAM_COOLDOWN;
        break;

    default:
        break;
    }

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
