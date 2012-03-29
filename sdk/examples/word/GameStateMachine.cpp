#include "GameStateMachine.h"
#include "Dictionary.h"
#include "EventID.h"
#include "EventData.h"
#include "SavedData.h"
#include "WordGame.h"
#include "assets.gen.h"


const float ANAGRAM_COOLDOWN = 2.0f; // TODO reduce when tilt bug is gone

GameStateMachine* GameStateMachine::sInstance = 0;

GameStateMachine::GameStateMachine(Cube cubes[]) :
    StateMachine(0), mAnagramCooldown(0.f), mTimeLeft(.0f), mScore(0),
    mNumAnagramsLeft(0), mCurrentMaxLettersPerCube(1), mNumHints(0)
{
    ASSERT(cubes != 0);
    sInstance = this;
    for (unsigned i = 0; i < arraysize(mLevelProgressData.mPuzzleProgress); ++i)
    {
        mLevelProgressData.mPuzzleProgress[i] = CheckMarkState_Hidden;
    }

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

unsigned GameStateMachine::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_GameStateChanged:
        if (data.mGameStateChanged.mNewStateIndex == GameStateIndex_PlayScored &&
            data.mGameStateChanged.mPreviousStateIndex != GameStateIndex_ShuffleScored)
        {
            // TODO different state for race mode or not
            mTimeLeft = 999999.0f;
#if (0)
#ifdef DEBUG
            mTimeLeft = (GameStateMachine::getCurrentMaxLettersPerCube() > 1) ? 999999.0f : 38.f;
#else
            mTimeLeft = (GameStateMachine::getCurrentMaxLettersPerCube() > 1) ? 999999.0f : 120.f;
#endif
#endif
            mAnagramCooldown = .0f;
            mScore = 0;
        }
        break;

    case EventID_NewPuzzle:
        mAnagramCooldown = ANAGRAM_COOLDOWN;
        // TODO data driven
        mNumAnagramsLeft = MAX(1, data.mNewPuzzle.mNumAnagrams);
        for (unsigned i = 0; i < arraysize(mLevelProgressData.mPuzzleProgress); ++i)
        {
            mLevelProgressData.mPuzzleProgress[i] =
                    (i < mNumAnagramsLeft) ?
                        CheckMarkState_Unchecked :
                        CheckMarkState_Hidden;
        }
        break;

    case EventID_NewWordFound:
        {
            unsigned len = _SYS_strnlen(data.mWordFound.mWord, 32);
            mScore += len;
            mNewWordLength = len;
            --mNumAnagramsLeft;

            // trade time for space, no "current index"
            for (unsigned i = 0; i < arraysize(mLevelProgressData.mPuzzleProgress); ++i)
            {
                if (mLevelProgressData.mPuzzleProgress[i] == CheckMarkState_Unchecked)
                {
                    mLevelProgressData.mPuzzleProgress[i] =
                            (data.mWordFound.mBonus) ?
                                CheckMarkState_CheckedBonus :
                                CheckMarkState_Checked;
                    break;
                }
                ASSERT(mLevelProgressData.mPuzzleProgress[i] != CheckMarkState_Hidden); // out of range, unexpected
            }

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

    unsigned result = StateMachine::onEvent(eventID, data);
    for (unsigned i = 0; i < arraysize(mCubeStateMachines); ++i)
    {
        mCubeStateMachines[i].onEvent(eventID, data);
    }
    return result;
}

unsigned GameStateMachine::sOnEvent(unsigned eventID, const EventData& data)
{
    if (sInstance != 0)
    {
        return sInstance->onEvent(eventID, data);
    }
    return 0;
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

    case GameStateIndex_StoryCityProgression:
        return mStoryCityProgressionState;
    }
}

void GameStateMachine::setState(unsigned newStateIndex, State& oldState)
{
    EventData data;
    data.mGameStateChanged.mPreviousStateIndex = getCurrentStateIndex();
    DEBUG_LOG(("GameStateMachine::setState: %d,\told: %d\n", newStateIndex, data.mGameStateChanged.mPreviousStateIndex));
    data.mGameStateChanged.mNewStateIndex = newStateIndex;
    StateMachine::setState(newStateIndex, oldState);
    onEvent(EventID_GameStateChanged, data);
}


unsigned GameStateMachine::getNumCubesInAnim(AnimType animT)
{
    ASSERT(sInstance);
    unsigned count = 0;
    for (unsigned i = 0; i < arraysize(sInstance->mCubeStateMachines); ++i)
    {
        if ((int)sInstance->mCubeStateMachines[i].getAnim() == animT)
        {
            ++count;
        }
    }
    return count;
}


unsigned GameStateMachine::getCurrentMaxLettersPerCube()
{
    ASSERT(sInstance);
    // TODO switch modes
    return sInstance->mCurrentMaxLettersPerCube;
}

void GameStateMachine::setCurrentMaxLettersPerCube(unsigned max)
{
    ASSERT(sInstance);
    sInstance->mCurrentMaxLettersPerCube = max;
}


unsigned GameStateMachine::getCurrentMaxLettersPerWord()
{
    ASSERT(sInstance);
    // TODO switch modes
    return sInstance->getCurrentMaxLettersPerCube() * NUM_CUBES;
}
