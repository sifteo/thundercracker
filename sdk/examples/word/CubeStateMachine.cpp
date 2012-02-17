#include "CubeStateMachine.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include "config.h"
#include "WordGame.h"
#include "assets.gen.h"

void CubeStateMachine::setCube(Cube& cube)
{
    mCube = &cube;
    for (unsigned i = 0; i < getNumStates(); ++i)
    {
        CubeState& state = (CubeState&)getState(i);
        state.setStateMachine(*this);
    }

    for (unsigned i = 0; i < arraysize(mTilePositions); ++i)
    {
        mTilePositions[i].x = 2 + i * 6;
        mTilePositions[i].y = 2;
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
    case EventID_Tilt:
        if (data.mInput.mCubeID == getCube().id())
        {
            switch (GameStateMachine::getCurrentMaxLettersPerCube())
            {
            case 2:
            case 3:
                if (!mBG0PanningLocked)
                {
                    const float BG0_PANNING_WRAP = 128.f;

                    _SYSTiltState state;
                    _SYS_getTilt(getCube().id(), &state);
                    if (state.x != 1)
                    {
                        mBG0TargetPanning -=
                                BG0_PANNING_WRAP/GameStateMachine::getCurrentMaxLettersPerCube() * (state.x - 1);
                        while (mBG0TargetPanning < 0.f)
                        {
                            mBG0TargetPanning += BG0_PANNING_WRAP;
                            mBG0Panning += BG0_PANNING_WRAP;
                        }
                        VidMode_BG0_SPR_BG1 vid(getCube().vbuf);
                        setPanning(vid, mBG0Panning);
                        WordGame::instance()->onEvent(EventID_LetterOrderChange, EventData());
                    }
                }
                break;

            default:
                break;
            }
        }
        // fall through
    case EventID_Input:
        if (data.mInput.mCubeID == mCube->id())
        {
            mIdleTime = 0.f;
        }
        break;

    case EventID_GameStateChanged:
        mBG0PanningLocked = (data.mGameStateChanged.mNewStateIndex != GameStateIndex_PlayScored);
        mBG0TargetPanning = 0.f;
        {
            VidMode_BG0_SPR_BG1 vid(getCube().vbuf);
            setPanning(vid, 0.f);
        }
        // fall through
    case EventID_EnterState:
    case EventID_AddNeighbor:
    case EventID_RemoveNeighbor:
        mIdleTime = 0.f;
        break;

    case EventID_NewAnagram:
        unsigned cubeIndex = (getCube().id() - CUBE_ID_BASE);
        for (unsigned i = 0; i < arraysize(mLetters); ++i)
        {
            mLetters[i] = '\0';
        }
        // TODO multiple letters: variable
        for (unsigned i = 0; i < GameStateMachine::getCurrentMaxLettersPerCube(); ++i)
        {
            mLetters[i] = data.mNewAnagram.mWord[cubeIndex * GameStateMachine::getCurrentMaxLettersPerCube() + i];
        }
        // TODO substrings of length 1 to 3
        break;
    }
    StateMachine::onEvent(eventID, data);
}


bool CubeStateMachine::getLetters(char *buffer, bool forPaint)
{
    ASSERT(mNumLetters > 0);
    if (mNumLetters <= 0)
    {
        return false;
    }
    switch (GameStateMachine::getCurrentMaxLettersPerCube())
    {
    case 2:
        if (!forPaint && fmodf(mBG0TargetPanning, 144.f) != 0.f)
        {
            char swapped[MAX_LETTERS_PER_CUBE + 1];
            swapped[0] = mLetters[1];
            swapped[1] = mLetters[0];
            swapped[2] = '\0';
            _SYS_strlcpy(buffer, swapped, GameStateMachine::getCurrentMaxLettersPerCube() + 1);
            return true;
        }
        _SYS_strlcpy(buffer, mLetters, GameStateMachine::getCurrentMaxLettersPerCube() + 1);
        return true;

    case 3:
        {
            float panMod = fmodf(mBG0TargetPanning, 144.f);
            if (!forPaint && panMod != 0.f)
            {
                char swapped[MAX_LETTERS_PER_CUBE + 1];
                if (panMod <= 72.f)
                {
                    swapped[0] = mLetters[1];
                    swapped[1] = mLetters[2];
                    swapped[2] = mLetters[0];
                    swapped[3] = '\0';
                }
                else
                {
                    swapped[0] = mLetters[2];
                    swapped[1] = mLetters[0];
                    swapped[2] = mLetters[1];
                    swapped[3] = '\0';
                }
                _SYS_strlcpy(buffer, swapped, GameStateMachine::getCurrentMaxLettersPerCube() + 1);
                return true;
            }
        }
        _SYS_strlcpy(buffer, mLetters, GameStateMachine::getCurrentMaxLettersPerCube() + 1);
        return true;

    default:
        _SYS_strlcpy(buffer, mLetters, GameStateMachine::getCurrentMaxLettersPerCube() + 1);
        return true;
    }
}

const Vec2& CubeStateMachine::geTilePosition(unsigned index) const
{
    return mTilePositions[index];
}

const AssetImage& CubeStateMachine::getTileAsset(unsigned index) const
{
    return Tile2;
}

bool CubeStateMachine::canBeginWord()
{
    return (mNumLetters > 0 &&
            mCube->physicalNeighborAt(SIDE_LEFT) == CUBE_ID_UNDEFINED &&
            mCube->physicalNeighborAt(SIDE_RIGHT) != CUBE_ID_UNDEFINED);
}

bool CubeStateMachine::beginsWord(bool& isOld, char* wordBuffer, bool& isBonus)
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
            char str[MAX_LETTERS_PER_CUBE + 1];
            csm->getLetters(str, false);
            _SYS_strlcat(wordBuffer, str, GameStateMachine::getCurrentMaxLettersPerWord() + 1);
            neighborLetters = true;
        }
        if (neighborLetters)
        {
            char trimmedWord[MAX_LETTERS_PER_WORD + 1];
            if (Dictionary::trim(wordBuffer, trimmedWord))
            {
                _SYS_strlcpy(wordBuffer, trimmedWord, sizeof trimmedWord);
                if (Dictionary::isWord(trimmedWord, isBonus))
                {
                    isOld = Dictionary::isOldWord(trimmedWord);
                    return true;
                }
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

void CubeStateMachine::update(float dt)
{
    mIdleTime += dt;
    StateMachine::update(dt);
    if ((int)mBG0Panning != (int)mBG0TargetPanning)
    {
        mBG0Panning += (mBG0TargetPanning - mBG0Panning) * dt * 7.5f;
        VidMode_BG0_SPR_BG1 vid(getCube().vbuf);
        setPanning(vid, mBG0Panning);
    }
}

void CubeStateMachine::setPanning(VidMode_BG0_SPR_BG1& vid, float panning)
{
    mBG0Panning = panning;
    int tilePanning = fmodf(mBG0Panning, 128.f);
    tilePanning /= 8;
    // TODO clean this up
    int tileWidth = 12/GameStateMachine::getCurrentMaxLettersPerCube();
    for (unsigned i = 0; i < GameStateMachine::getCurrentMaxLettersPerCube(); ++i)
    {
        mTilePositions[i].x = 2 + tilePanning + i * tileWidth;
        mTilePositions[i].x = ((mTilePositions[i].x + tileWidth) % (16 + 2 * tileWidth));
        mTilePositions[i].x -= tileWidth;
    }
    //vid.BG0_setPanning(Vec2((int)mBG0Panning, 0.f));
}
