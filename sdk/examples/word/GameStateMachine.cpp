#include "GameStateMachine.h"
#include "Dictionary.h"
#include "EventID.h"
#include "EventData.h"
#include "SavedData.h"
#include "WordGame.h"
#include "assets.gen.h"


const float ANAGRAM_COOLDOWN = 2.0f; // TODO reduce when tilt bug is gone
#ifdef DEBUG

float ROUND_TIME = (MAX_LETTERS_PER_CUBE > 1) ? 999999.0f : 38.f;
#else
float ROUND_TIME = (MAX_LETTERS_PER_CUBE > 1) ? 120.f : 999999.0f;
#endif

GameStateMachine* GameStateMachine::sInstance = 0;

GameStateMachine::GameStateMachine(Cube cubes[]) :
    StateMachine(0), mAnagramCooldown(0.f), mTimeLeft(.0f), mScore(0),
    mNumAnagramsRemaining(0)
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
        if (data.mGameStateChanged.mNewStateIndex == GameStateIndex_PlayScored &&
            data.mGameStateChanged.mPreviousStateIndex != GameStateIndex_ShuffleScored)
        {
            mTimeLeft = ROUND_TIME;
            mAnagramCooldown = .0f;
            mScore = 0;
        }
        break;

    case EventID_NewAnagram:
        mAnagramCooldown = ANAGRAM_COOLDOWN;
        mNumAnagramsRemaining = data.mNewAnagram.mNumAnagrams;
        break;

    case EventID_NewWordFound:
        {
            unsigned len = _SYS_strnlen(data.mWordFound.mWord, 32);
            mScore += len;
            mNewWordLength = len;
            --mNumAnagramsRemaining;
            // TODO multiple letters per cube
            // TODO count active cubes
            /* TODO extra time sound
            if (len >= 4)
            {
                mTimeLeft += len - 2;
                WordGame::playAudio(extratime, AudioChannelIndex_Bonus);
            }
            */
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


unsigned GameStateMachine::getNumCubesInState(CubeStateIndex stateIndex)
{
    ASSERT(sInstance);
    unsigned count = 0;
    for (unsigned i = 0; i < arraysize(sInstance->mCubeStateMachines); ++i)
    {
        if (sInstance->mCubeStateMachines[i].getCurrentStateIndex() == stateIndex)
        {
            ++count;
        }
    }
    return count;
}
