#include "CubeStateMachine.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include <string.h>
#include "GameStateMachine.h"

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

void CubeStateMachine::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_NewAnagram:
        uint8_t cubeID = getCube().id();
        for (unsigned i = 0; i < arraysize(mLetters); ++i)
        {
            mLetters[i] = '\0';
        }
        for (unsigned i = 0; i < mNumLetters; ++i)
        {
            mLetters[i] = data.mNewAnagram.mWord[cubeID + i];
        }
        // TODO substrings of length 1 to 3
        break;
    }
    StateMachine::onEvent(eventID, data);
}


const char* CubeStateMachine::getLetters()
{
    ASSERT(mNumLetters > 0);
    return mLetters;
}

bool CubeStateMachine::canBeginWord()
{
    // TODO vertical words
    return (mNumLetters > 0 &&
            mCube->physicalNeighborAt(SIDE_LEFT) == CUBE_ID_UNDEFINED &&
            mCube->physicalNeighborAt(SIDE_RIGHT) != CUBE_ID_UNDEFINED);
}

bool CubeStateMachine::beginsWord(bool& isOld, char* wordBuffer)
{
    if (canBeginWord())
    {        
        wordBuffer[0] = '\0';
        CubeStateMachine* csm = this;
        bool neighborLetters = false;
        for (Cube::ID neighborID = csm->mCube->physicalNeighborAt(SIDE_RIGHT);
             csm && neighborID != CUBE_ID_UNDEFINED;
             neighborID = csm->mCube->physicalNeighborAt(SIDE_RIGHT),
             csm = GameStateMachine::findCSMFromID(neighborID))
        {
            if (csm->mNumLetters <= 0)
            {
                break;
            }
            strcat(wordBuffer, csm->mLetters);
            neighborLetters = true;
        }
        if (neighborLetters)
        {
            if (Dictionary::isWord(wordBuffer))
            {
                isOld = Dictionary::isOldWord(wordBuffer);
                return true;
            }
        }
    }
    return false;
}

unsigned CubeStateMachine::findRowLength()
{
    unsigned result = 1;
    for (Cube::Side side = SIDE_LEFT; side <= SIDE_RIGHT; side +=2)
    {
        CubeStateMachine* csm = this;
        for (Cube::ID neighborID = csm->mCube->physicalNeighborAt(side);
             csm && neighborID != CUBE_ID_UNDEFINED;
             neighborID = csm->mCube->physicalNeighborAt(side),
             csm = GameStateMachine::findCSMFromID(neighborID))
        {
            if (csm != this)
            {
                ++result;
            }
        }
    }

    return result;
}

bool CubeStateMachine::hasNoNeighbors() const
{
    for (Cube::Side side = 0; side < NUM_SIDES; ++side)
    {
        if (mCube->physicalNeighborAt(side) != CUBE_ID_UNDEFINED)
        {
            return false;
        }
    }
    return true;
}

bool CubeStateMachine::isConnectedToCubeOnSide(Cube::ID cubeIDStart,
                                               Cube::Side side)
{
    if (mCube->id() == cubeIDStart)
    {
        return true;
    }

    CubeStateMachine* csm = this;
    for (Cube::ID neighborID = csm->mCube->physicalNeighborAt(side);
         csm && neighborID != CUBE_ID_UNDEFINED;
         neighborID = csm->mCube->physicalNeighborAt(side),
         csm = GameStateMachine::findCSMFromID(neighborID))
    {
        // only check the left most letter, as it it the one that
        // determines the entire word state
        if (neighborID == cubeIDStart)
        {
            return true;
        }
    }
    return false;
}

State& CubeStateMachine::getState(unsigned index)
{
    ASSERT(index < getNumStates());
    switch (index)
    {
    case CubeStateIndex_Title:
        return mTitleState;

    case CubeStateIndex_TitleExit:
        return mTitleExitState;

    default:
        ASSERT(0);
        // fall through
    case CubeStateIndex_NotWordScored:
        return mNotWordScoredState;

    case CubeStateIndex_NewWordScored:
        return mNewWordScoredState;

    case CubeStateIndex_OldWordScored:
        return mOldWordScoredState;

    case CubeStateIndex_StartOfRoundScored:
        return mStartOfRoundScoredState;

    case CubeStateIndex_EndOfRoundScored:
        return mEndOfRoundScoredState;

    case CubeStateIndex_ShuffleScored:
        return mShuffleScoredState;
    }
}

unsigned CubeStateMachine::getNumStates() const
{
    return CubeStateIndex_NumStates;
}
