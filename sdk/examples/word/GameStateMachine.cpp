#include "GameStateMachine.h"
#include "Dictionary.h"
#include "EventID.h"
#include "EventData.h"
#include <string.h>
#include "SavedData.h"
#include "WordGame.h"
#include "audio.gen.h"

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
    unsigned oldSecsLeft = getSecondsLeft();
    mTimeLeft -= dt;
    mTimeLeft = MAX(.0f, mTimeLeft);

    if (oldSecsLeft != getSecondsLeft())
    {
        // TODO dirty flags?
        onEvent(EventID_ClockTick, EventData());
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
    case EventID_GameStateChanged:
        if (data.mGameStateChanged.mNewStateIndex == GameStateIndex_StartOfRoundScored)
        {
            mTimeLeft = ROUND_TIME;
            mAnagramCooldown = .0f;
            mScore = 0;
        }
        break;

    case EventID_NewAnagram:
        mAnagramCooldown = ANAGRAM_COOLDOWN;
        break;

    case EventID_NewWordFound:
        {
            unsigned len = strlen(data.mWordFound.mWord);
            mScore += len;
            // TODO multiple letters per cube
            // TODO count active cubes
            if (len >= 4)
            {
                mTimeLeft += len - 2;
                WordGame::playAudio(extratime, AudioChannelIndex_Bonus);
            }
        }
        break;

    default:
        break;
    }
    Dictionary::sOnEvent(eventID, data);
    SavedData::sOnEvent(eventID, data);

    StateMachine::onEvent(eventID, data);
    for (unsigned i = 0; i < arraysize(mCubeStateMachines); ++i)
    {
        mCubeStateMachines[i].onEvent(eventID, data);
    }
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
    ASSERT(index < GameStateIndex_NumStates);
    switch (index)
    {
    case GameStateIndex_Title:
        return mTitleState;
    default:
        ASSERT(0);
        // fall through
    case GameStateIndex_PlayScored:
        return mScoredState;

    case GameStateIndex_StartOfRoundScored:
        return mScoredStartOfRoundState;

    case GameStateIndex_EndOfRoundScored:
        return mScoredEndOfRoundState;

    case GameStateIndex_ShuffleScored:
        return mScoredShuffleState;
    }
}

void GameStateMachine::setState(unsigned newStateIndex, State& oldState)
{
    EventData data;
    data.mGameStateChanged.mPreviousStateIndex = getCurrentStateIndex();
    data.mGameStateChanged.mNewStateIndex = newStateIndex;
    StateMachine::setState(newStateIndex, oldState);
    onEvent(EventID_GameStateChanged, data);
}
