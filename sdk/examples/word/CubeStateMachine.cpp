#include <sifteo.h>
#include "CubeStateMachine.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include "config.h"
#include "WordGame.h"
#include "assets.gen.h"
#include "CubeState.h"

const float SHAKE_DELAY = 3.5f;

CubeStateMachine::CubeStateMachine() :
        StateMachine(CubeStateIndex_Menu), mPuzzleLettersPerCube(0),
        mPuzzlePieceIndex(0), mMetaLettersPerCube(0), mIdleTime(0.f),
        mNewHint(false), mPainting(false), mBG0Panning(0.f),
        mBG0TargetPanning(0.f), mBG0PanningLocked(true), mLettersStart(0),
        mLettersStartOld(0), mVidBuf(0),
        mShakeDelay(0.f), mPanning(0.f), mTouchHoldTime(0.f),
        mTouchHoldWaitForUntouch(false)

{
    mLetters[0] = '\0';
    mHintSolution[0] = '\0';
    mMetaLetters[0] = '\0';

    for (unsigned i = 0; i < arraysize(mAnimTypes); ++i)
    {
        mAnimTypes[i] = AnimType_None;
    }

    for (unsigned i = 0; i < arraysize(mAnimTimes); ++i)
    {
        mAnimTimes[i] = 0.f;
    }
}

void CubeStateMachine::setVideoBuffer(VideoBuffer& vidBuf)
{
    mVidBuf = &vidBuf;

    for (unsigned i = 0; i < arraysize(mTilePositions); ++i)
    {
        mTilePositions[i].x = 2 + i * 6;
        mTilePositions[i].y = 2;
    }
}

CubeID CubeStateMachine::getCube()
{
    ASSERT(mVidBuf != 0);
    return mVidBuf->cube();
}

unsigned CubeStateMachine::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_Paint:
    case EventID_ClockTick:
        // FIXME only paint if required? does it matter, since the engine
        // checks if anything really needs to update?
        paint();
        break;

    case EventID_Touch:
        if (data.mInput.mCubeID == getCube() && !mTouchHoldWaitForUntouch)
        {
            mTouchHoldTime = 0.f;
        }
        break;

    case EventID_TouchAndHoldWaitForUntouch:
        if (data.mTouchAndHoldWaitForUntouch.mCubeID == getCube())
        {
            mTouchHoldWaitForUntouch = true;
        }
        break;

    case EventID_Tilt:
        switch (mAnimTypes[CubeAnim_Main])
        {
        default:
        case AnimType_SlideL:
        case AnimType_SlideR:
            break;

        case AnimType_NotWord:
        case AnimType_OldWord:
        case AnimType_NewWord:

            if (data.mInput.mCubeID == getCube())
            {
                switch (GameStateMachine::getCurrentMaxLettersPerCube())
                {
                case 2:
                case 3:
                    if (!mBG0PanningLocked)
                    {
                        const float BG0_PANNING_WRAP = 144.f;

                        Byte2 tilt = getCube().tilt();
                        if (tilt.x != 1)
                        {

                            mBG0TargetPanning -=
                                    BG0_PANNING_WRAP/GameStateMachine::getCurrentMaxLettersPerCube() * (tilt.x - 1);
                            while (mBG0TargetPanning < 0.f)
                            {
                                mBG0TargetPanning += BG0_PANNING_WRAP;
                                mBG0Panning += BG0_PANNING_WRAP;
                            }

                            ASSERT(mVidBuf != NULL);
                            setPanning(mBG0Panning);
                            if (tilt.x < 1)
                            {
                                queueAnim(AnimType_SlideL);//, vid); // FIXME
                            }
                            else
                            {
                                queueAnim(AnimType_SlideR);//, vid); // FIXME
                            }

                            unsigned char start;
                            unsigned char lpc;
                            if (Dictionary::currentIsMetaPuzzle())
                            {
                                start = mMetaLettersStart;
                                lpc = mMetaLettersPerCube;
                            }
                            else
                            {
                                start = mLettersStart;
                                lpc = mPuzzleLettersPerCube;

                            }
                            unsigned newStart = (tilt.x == 0) ? start + 1 : start + (lpc - 1);
                            newStart = (newStart % lpc);
                            setLettersStart(newStart);
                            // letters are unavailable until anim finishes, but
                            // need to break word now
                            WordGame::instance()->onEvent(EventID_LetterOrderChange, EventData());
                        }
                    }
                    break;

                default:
                    break;
                }
            }
            if (data.mInput.mCubeID == getCube())
            {
                mIdleTime = 0.f;
            }
            break;
        }
        break;

    case EventID_Shake:
        if (data.mInput.mCubeID == getCube())
        {
            mIdleTime = 0.f;
        }
        break;

    case EventID_GameStateChanged:
        mBG0PanningLocked = (data.mGameStateChanged.mNewStateIndex != GameStateIndex_PlayScored);
        mBG0TargetPanning = 0.f;
        {
            setPanning(0.f);
            mVidBuf->bg1.setPanning(vec(0, 0));
        }
        mIdleTime = 0.f;
        switch (data.mGameStateChanged.mNewStateIndex)
        {
        case GameStateIndex_EndOfRoundScored:
            queueAnim(AnimType_EndofRound);
            break;

        case GameStateIndex_ShuffleScored:
            queueAnim(AnimType_Shuffle);
            break;

        case GameStateIndex_StoryStartOfRound:
            queueAnim(AnimType_MetaTilesShow);
            break;

        case GameStateIndex_PlayScored:
            break;

        }
        break;

    case EventID_EnterState:
        //queueNextAnim();//vid, bg1, params);
        mIdleTime = 0.f;
        paint();
        WordGame::instance()->setNeedsPaintSync();
        break;

    case EventID_AddNeighbor:
    case EventID_RemoveNeighbor:
    case EventID_LetterOrderChange:
        switch (mAnimTypes[CubeAnim_Hint])
        {
        default:
            break;

        case AnimType_HintWindUpSlide:
            queueAnim(AnimType_HintBarIdle, CubeAnim_Hint);
            break;

        case AnimType_HintSlideL:
        case AnimType_HintSlideR:
            {
                unsigned newStart = 0;
                unsigned tiltDir = 0;
                if (!calcHintTiltDirection(newStart, tiltDir))
                {
                    queueAnim(AnimType_HintBarIdle, CubeAnim_Hint);
                }
            }
            break;
        }

        switch (mAnimTypes[CubeAnim_Main])
        {
        default:
        case AnimType_NewWord: // see ::update (wait for min display time)
            break;

        case AnimType_NotWord:
        case AnimType_OldWord:
            {
                bool isOldWord = false;
                if (canBeginWord())
                {
                    char wordBuffer[MAX_LETTERS_PER_WORD + 1];
                    EventData wordFoundData;
                    if (beginsWord(isOldWord, wordBuffer, wordFoundData.mWordFound.mBonus))
                    {
                        wordFoundData.mWordFound.mCubeIDStart = getCube();
                        wordFoundData.mWordFound.mWord = wordBuffer;
                        if (isOldWord)
                        {
                            GameStateMachine::sOnEvent(EventID_OldWordFound, wordFoundData);                            
                            queueAnim(AnimType_OldWord);
                        }
                        else
                        {
                            GameStateMachine::sOnEvent(EventID_NewWordFound, wordFoundData);
                            queueAnim(AnimType_NewWord);
                        }
                    }
                    else
                    {
                        EventData wordBrokenData;
                        wordBrokenData.mWordBroken.mCubeIDStart = getCube();
                        GameStateMachine::sOnEvent(EventID_WordBroken, wordBrokenData);
                        queueAnim(AnimType_NotWord);
                    }
                }
                else if (hasNoNeighbors())
                {
                    queueAnim(AnimType_NotWord);
                }
            }
            break;
        }

        mIdleTime = 0.f;
        paint();
        break;

    case EventID_NewWordFound:
        switch (mAnimTypes[CubeAnim_Main])
        {
        case AnimType_NewWord:
            if (isConnectedToCubeOnSide(data.mWordFound.mCubeIDStart))
            {
                resetStateTime();
            }
            break;

        default:
            if (!canBeginWord() &&
                 isConnectedToCubeOnSide(data.mWordFound.mCubeIDStart))
            {
                queueAnim(AnimType_NewWord);
            }
            break;
        }
        queueAnim(AnimType_HintBarIdle, CubeAnim_Hint);
        paint();
        break;

    case EventID_OldWordFound:
        if (!canBeginWord() &&
             isConnectedToCubeOnSide(data.mWordFound.mCubeIDStart))
        {
            queueAnim(AnimType_OldWord);
        }
        break;

    case EventID_WordBroken:
        switch (getAnim())
        {
        case AnimType_OldWord:
        case AnimType_NewWord:
            if (!canBeginWord() &&
                isConnectedToCubeOnSide(data.mWordBroken.mCubeIDStart))
            {
                queueAnim(AnimType_NotWord);
            }
            break;

        default:
            break;
        }
        break;

    case EventID_NewPuzzle:
        {
            unsigned cubeIndex = (getCube() - CUBE_ID_BASE);
            mPuzzlePieceIndex = data.mNewPuzzle.mCubeOrderingIndexes[cubeIndex];
            setLettersStart(data.mNewPuzzle.mLetterStartIndexes[cubeIndex]);
            for (unsigned i = 0; i < arraysize(mLetters); ++i)
            {
                mLetters[i] = '\0';
            }

            // TODO multiple letters: variable
            for (unsigned i = 0; i < data.mNewPuzzle.mMaxLettersPerCube; ++i)
            {
                mLetters[i] =
                        data.mNewPuzzle.mWord[mPuzzlePieceIndex * data.mNewPuzzle.mMaxLettersPerCube + i];
            }
            mPuzzleLettersPerCube = data.mNewPuzzle.mMaxLettersPerCube; // FIXME this var name is misleading
            // TODO substrings of length 1 to 3
            paint();
        }
        break;

    case EventID_NewMeta:
        {
            unsigned cubeIndex = (getCube() - CUBE_ID_BASE);
            mMetaPieceIndex = data.mNewMeta.mCubeOrderingIndexes[cubeIndex];
            mMetaLettersStart = data.mNewMeta.mLetterStartIndexes[cubeIndex];
            for (unsigned i = 0; i < arraysize(mMetaLetters); ++i)
            {
                mMetaLetters[i] = '\0';
            }
            mMetaLettersPerCube = data.mNewMeta.mMaxLettersPerCube;

            // TODO multiple letters: variable
            for (unsigned i = 0; i < mMetaLettersPerCube; ++i)
            {
                mMetaLetters[i] =
                        data.mNewMeta.mWord[mMetaPieceIndex * mMetaLettersPerCube + i];
            }
            // TODO substrings of length 1 to 3
            paint();
        }
        break;

    case EventID_NormalTilesReveal:
        switch (getAnim())
        {
        case AnimType_NormalTilesReveal:
        case AnimType_NewWord:
            break;

        default:
            queueAnim(AnimType_NormalTilesReveal);
            break;
        }
        break;

    case EventID_HintSolutionUpdated:
        {
            _SYS_strlcpy(mHintSolution,
                         data.mHintSolutionUpdate.mHintSolution[Dictionary::currentIsMetaPuzzle() ? mMetaPieceIndex : mPuzzlePieceIndex],
                         sizeof(mHintSolution));
        }
        break;

    case EventID_SpendHint:
        switch (mAnimTypes[CubeAnim_Hint])
        {
        case AnimType_HintWindUpSlide:
            queueAnim(AnimType_HintBarIdle, CubeAnim_Hint);
            break;

        default:
            break;
        }
        break;
    }

    unsigned newStateIndex = getCurrentStateIndex();
    switch (newStateIndex)
    {
    case CubeStateIndex_Menu:
        switch (eventID)
        {
        case EventID_GameStateChanged:
            // TODO drive machine by anim state only
            switch (data.mGameStateChanged.mNewStateIndex)
            {
            case GameStateIndex_PlayScored:
                newStateIndex = CubeStateIndex_NotWordScored;
                break;
            case GameStateIndex_Title:
                newStateIndex = CubeStateIndex_Title;
                break;

            case GameStateIndex_StoryStartOfRound:
                newStateIndex = CubeStateIndex_StoryStartOfRound;
                break;

            default:
                break;
            }
            break;

        default:
            break;
        }
        break;

    case CubeStateIndex_Title:
        switch (eventID)
        {
        // TODO debug: case EventID_Paint:
        case EventID_EnterState:
            mShakeDelay = 0.f;
            mPanning = -16.f;// * ((getCube() & 1) ? -1.f : 1.f);
            {
                BG1Mask bg1;
                bg1.filled(vec(0,2), vec(15, 8));
                mVidBuf->bg1.setMask(bg1);
            }
            paint();
            if (eventID == EventID_EnterState)
            {
                WordGame::instance()->setNeedsPaintSync();
            }
            break;

        case EventID_Paint:
            paint();
            break;

        case EventID_GameStateChanged:
            switch (data.mGameStateChanged.mNewStateIndex)
            {
            case GameStateIndex_StoryStartOfRound:
                newStateIndex = CubeStateIndex_StoryStartOfRound;
                break;

            case GameStateIndex_StartOfRoundScored:
                newStateIndex = CubeStateIndex_StartOfRoundScored;
                break;

            case GameStateIndex_PlayScored:
                newStateIndex = CubeStateIndex_NotWordScored;
                break;
            }
            break;

        case EventID_Update:
            {
                float dt = data.mUpdate.mDT;
                mShakeDelay -= dt;
                if (mShakeDelay <= 0.f)
                {
                    mShakeDelay = SHAKE_DELAY;
                }

                Byte3 accelState = getCube().accel();
                /* TODO remove _SYSTiltState tiltState;
                _SYS_getTilt(getCube(), &tiltState);
            */
                if (!getCube().isShaking() && abs<int>(accelState.x) > 10)
                {
                    mShakeDelay = 0.f;
                    mPanning += dt * -5.f * accelState.x; // FIXME treat as accel, not velocity set
                }
                /*if (mPanning != 0.f)
                {
                    LOG(("panning %f\n", mPanning));
                }*/
                //mPanning = fmodf(mPanning, 128.f);
                if (abs(mPanning) > 86.f)
                {
                    GameStateMachine::sOnEvent(EventID_Start, EventData());
                    // refresh after event handling
                    newStateIndex = getCurrentStateIndex();
                }
            }
            break;
        }
        break;

    case CubeStateIndex_TitleExit:
        switch (eventID)
        {
        // TODO debug: case EventID_Paint:
        case EventID_EnterState:
        case EventID_Paint:
            paint();
            break;

        case EventID_Update:
            newStateIndex = mStateTime > TRANSITION_ANIM_LENGTH ? CubeStateIndex_NotWordScored : CubeStateIndex_TitleExit;
            break;
        }
        break;

    case CubeStateIndex_StoryCityProgression:
        switch (eventID)
        {
        case EventID_EnterState:
            break;
        }
        break;

    case CubeStateIndex_OldWordScored:
        break;

    case CubeStateIndex_StartOfRoundScored:
    case CubeStateIndex_StoryStartOfRound:
        switch (eventID)
        {
        case EventID_EnterState:
        case EventID_NewPuzzle:
        case EventID_Paint:
        case EventID_ClockTick:
            paint();
            break;

        case EventID_GameStateChanged:
            switch (data.mGameStateChanged.mNewStateIndex)
            {
            case GameStateIndex_PlayScored:
                newStateIndex = CubeStateIndex_NotWordScored;
            }
            break;
        }
        break;

    case CubeStateIndex_EndOfRoundScored:
        switch (eventID)
        {
        case EventID_EnterState:
        case EventID_Paint:
            paint();
            break;

        case EventID_GameStateChanged:
            switch (data.mGameStateChanged.mNewStateIndex)
            {
            case GameStateIndex_StartOfRoundScored:
                newStateIndex = CubeStateIndex_StartOfRoundScored;
            }
            break;
        }
        break;

    case CubeStateIndex_ShuffleScored:
        switch (eventID)
        {
        case EventID_EnterState:
            WordGame::instance()->setNeedsPaintSync();
            // fall through
        case EventID_Paint:
            paint();
            break;
        case EventID_Update:
            newStateIndex =
                    mStateTime <= 0.5f ?
                        CubeStateIndex_ShuffleScored :
                        CubeStateIndex_NotWordScored;
            break;
        }
        break;

    case CubeStateIndex_NotWordScored:
        switch (eventID)
        {
        case EventID_GameStateChanged:
            switch (data.mGameStateChanged.mNewStateIndex)
            {
            case GameStateIndex_PauseMenu:
            case GameStateIndex_MainMenu:
                {
                    CubeID c = getCube();
                    ASSERT(mVidBuf != NULL);
                    WordGame::hideSprites(*mVidBuf);
                    newStateIndex = CubeStateIndex_Menu;
                }
                break;

            case GameStateIndex_StoryCityProgression:
                newStateIndex = CubeStateIndex_StoryCityProgression;
                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }

   if (newStateIndex != getCurrentStateIndex())
   {
       ASSERT(eventID != EventID_EnterState && eventID != EventID_ExitState); // don't change states while changing states
       setState(newStateIndex, getCurrentStateIndex());
   }

    return newStateIndex;
}

unsigned CubeStateMachine::findNumLetters(char *string)
{
    if (string == 0)
    {
        return 0;
    }

    unsigned count = 0;
    for (unsigned i=0; i<GameStateMachine::getCurrentMaxLettersPerCube(); ++i)
    {
        if (string[i] < 'A' || string[i] > 'Z')
        {
            return count;
        }
        ++count;
    }
    return count;
}

unsigned CubeStateMachine::getLetters(char *buffer, bool forPaint) const
{
    if (Dictionary::currentIsMetaPuzzle())
    {
        return getMetaLetters(buffer, forPaint);
    }

    if (mPuzzleLettersPerCube <= 0)
    {
        return 0;
    }

    unsigned start = mLettersStart;
    const char *letters = mLetters;
    unsigned lettersPerCube = mPuzzleLettersPerCube;
    ASSERT(lettersPerCube <= MAX_LETTERS_PER_CUBE);
    switch (mAnimTypes[CubeAnim_Main])
    {
    case AnimType_SlideL:
    case AnimType_SlideR:
        if (!forPaint)
        {
            return 0;
        }
        start = mLettersStartOld;
        // fall through
    default:
        if (start == 0)
        {
            _SYS_strlcpy(buffer, letters, lettersPerCube + 1);
        }
        else
        {
            //LOG(("letters start: %d\n", mLettersStart));
            // copy from the (offset) start to the end of the letters
            _SYS_strlcpy(buffer, &letters[start], lettersPerCube + 1 - start);
            // fill in the rest of the buffer with the substring of the letters
            // that came after the end of the mLetters buffer, and zero terminate
            _SYS_strlcat(buffer, letters, lettersPerCube + 1);
        }
        break;
    }

    return _SYS_strnlen(buffer, lettersPerCube);
}

unsigned CubeStateMachine::getMetaLetters(char *buffer, bool forPaint) const
{
    if (mMetaLettersPerCube <= 0)
    {
        return 0;
    }

    unsigned start = mMetaLettersStart;
    const char *letters = mMetaLetters;
    unsigned lettersPerCube = mMetaLettersPerCube;
    ASSERT(lettersPerCube <= MAX_LETTERS_PER_CUBE);
    switch (mAnimTypes[CubeAnim_Main])
    {
    case AnimType_SlideL:
    case AnimType_SlideR:
        if (!forPaint)
        {
            return 0;
        }
        start = mMetaLettersStartOld;
        // fall through
    default:
        for (unsigned i = 0; i < lettersPerCube; ++i)
        {
            unsigned src = (i + start) % lettersPerCube;
            if (GameStateMachine::getInstance().isMetaLetterIndexUnlocked(src + mMetaLettersPerCube * mMetaPieceIndex))
            {
                buffer[i] = letters[src];
            }
            else
            {
                buffer[i] = 'Z' + 1; // '?' in the strip
            }
        }
        break;
    }

    buffer[lettersPerCube] = '\0';
    return lettersPerCube;
}

void CubeStateMachine::queueAnim(AnimType anim, CubeAnim cubeAnim)
{
    LOG("queue anim cube ID: %d,\tAnimType: %d,\tCubeAnim:\t%d\n",
               (PCubeID)getCube(), anim, cubeAnim);

    mAnimTypes[cubeAnim] = anim;
    mAnimTimes[cubeAnim] = 0.f;
    WordGame::instance()->setNeedsPaintSync();

    // FIXME params aren't really sent through right now: animPaint(anim, vid, bg1, mAnimTime, params);

    switch (anim)
    {
    default:
        break;

    case AnimType_NormalTilesReveal:
    case AnimType_MetaTilesReveal:
        // setup sprite params
        /* TODO remove for (unsigned i=0; i<arraysize(mSpriteParams.mPositions); ++i)
        {
            calcSpriteParams(i);
        }
        */
        break;

    case AnimType_NewWord:
        {
            CubeID c = getCube();

            // setup sprite params
            for (unsigned i=0; i<arraysize(mSpriteParams.mPositions); ++i)
            {
                calcSpriteParams(i);
            }

        }
        //WordGame::instance()->setNeedsPaintSync();
        break;
    }
}

void CubeStateMachine::updateSpriteParams(float dt)
{
    for (unsigned i=0; i<arraysize(mSpriteParams.mPositions); ++i)
    {
        // eased approach
        const Float2 &v =
                (mSpriteParams.mEndPositions[i] - mSpriteParams.mPositions[i]);
        if (mSpriteParams.mLoop[i] && v.len2() < 2.f)
        {
            calcSpriteParams(i);
        }
        else if (mSpriteParams.mStartDelay[i] <= 0.f)
        {
            mSpriteParams.mPositions[i] += mSpriteParams.mSpeeds[i] * dt * v;
        }
        else
        {
            mSpriteParams.mStartDelay[i] -= dt;
        }
    }
}

void CubeStateMachine::calcSpriteParams(unsigned i)
{
    switch (getAnim())
    {
    default:
    case AnimType_NewWord:
        {
            mSpriteParams.mPositions[i].x = 56.f;
            mSpriteParams.mPositions[i].y = 56.f;
            float angle = i * M_PI_4;
            mSpriteParams.mEndPositions[i].setPolar(WordGame::random.uniform(angle * .75f,
                                                                             angle * 1.25f),
                                                    WordGame::random.uniform(32.f, 52.f));
            mSpriteParams.mEndPositions[i] += vec(56.f, 56.f);
            mSpriteParams.mStartDelay[i] = WordGame::random.random() * 0.5f;
            mSpriteParams.mSpeeds[i] = 5.5f + WordGame::random.random() * 1.f;
            mSpriteParams.mLoop[i] = true;
        }
        break;

    case AnimType_NormalTilesReveal:
    case AnimType_MetaTilesReveal:
        {
            unsigned char lpc;
            unsigned char tileIndex;
            unsigned char firstTileIndex;
            unsigned char startOffset;
            if (getAnim() == AnimType_NormalTilesReveal)
            {
                lpc = mPuzzleLettersPerCube;
                tileIndex = Dictionary::getCurrentPuzzleMetaLetterIndex();
                firstTileIndex = mPuzzlePieceIndex * lpc;
                startOffset = mLettersStart;
            }
            else
            {
                lpc = mMetaLettersPerCube;
                tileIndex = 0; // TODO
                firstTileIndex = mMetaPieceIndex * lpc;
                startOffset = mMetaLettersStart;
            }

            if (tileIndex >= firstTileIndex && tileIndex < firstTileIndex + lpc)
            {
                mSpriteParams.mPositions[i].x =
                        Sparkle.pixelWidth() * i +
                        ((tileIndex - firstTileIndex + startOffset) % lpc) * 96/lpc + 16.f;
                mSpriteParams.mEndPositions[i].x = mSpriteParams.mPositions[i].x;
                if (getAnim() == AnimType_NormalTilesReveal)
                {
                    mSpriteParams.mPositions[i].y = 16.f;
                    mSpriteParams.mEndPositions[i].y =
                            LCD_height - (Sparkle.pixelHeight() + 16.f);
                }
                else
                {
                    mSpriteParams.mEndPositions[i].y = 16.f;
                    mSpriteParams.mPositions[i].y =
                            LCD_height - (Sparkle.pixelHeight() + 16.f);
                }
                mSpriteParams.mSpeeds[i] = 1.0f;
                mSpriteParams.mStartDelay[i] = 0.f;
            }
            else
            {
                mSpriteParams.mStartDelay[i] = 99999999.f;
            }
            mSpriteParams.mLoop[i] = false;
        }
        break;
    }
}

void CubeStateMachine::queueNextAnim(CubeAnim cubeAnim)
{
    AnimType oldAnim = mAnimTypes[cubeAnim];
    AnimType anim = getNextAnim(cubeAnim);
    queueAnim(anim, cubeAnim);//, vid, bg1, params);
    switch (oldAnim)
    {
    case AnimType_SlideL:
    case AnimType_SlideR:
        WordGame::instance()->onEvent(EventID_LetterOrderChange, EventData());
        break;

    case AnimType_NewWord:
        // FIXME do this kind of handling out of OldWord too
        switch (anim)
        {
        default:
            break;

        case AnimType_NormalTilesReveal:
            WordGame::instance()->onEvent(EventID_NormalTilesReveal, EventData());
            break;

        case AnimType_OldWord:
        case AnimType_NewWord:
            {
                char wordBuffer[MAX_LETTERS_PER_WORD + 1];
                EventData wordFoundData;
                bool isOldWord = false;
                if (beginsWord(isOldWord, wordBuffer, wordFoundData.mWordFound.mBonus))
                {
                    wordFoundData.mWordFound.mCubeIDStart = getCube();
                    wordFoundData.mWordFound.mWord = wordBuffer;

                    if (anim == AnimType_OldWord)
                    {
                        GameStateMachine::sOnEvent(EventID_OldWordFound, wordFoundData);
                    }
                    else
                    {
                        GameStateMachine::sOnEvent(EventID_NewWordFound, wordFoundData);
                    }
                }
            }
            break;

        case AnimType_NotWord:
            {
                EventData wordBrokenData;
                wordBrokenData.mWordBroken.mCubeIDStart = getCube();
                GameStateMachine::sOnEvent(EventID_WordBroken, wordBrokenData);
            }
            break;
        }
        break;

    default:
        break;
    }

    switch (anim)
    {
    case AnimType_HintSlideL:
    case AnimType_HintSlideR:
        WordGame::instance()->onEvent(EventID_SpendHint, EventData());
        break;


    default:
        break;
    }
}

void CubeStateMachine::updateAnim(TileBuffer<16,16,1> &bg1TileBuf, AnimParams *params)
{
    for (unsigned i = 0; i < NumCubeAnims; ++i)
    {
        if (params)
        {
            params->mCubeAnim = i;
        }
        if (mAnimTypes[i] != AnimType_None &&
            !animPaint(mAnimTypes[i], *mVidBuf, bg1TileBuf, mAnimTimes[i], params))
        {
            queueNextAnim((CubeAnim)i);//, vid, bg1, params);
        }
    }
}

bool CubeStateMachine::canBeginWord()
{
    // check if this is the left-most cube in a neighbor chain
    Neighborhood hood(getCube());
    return (mPuzzleLettersPerCube > 0 &&
            hood.neighborAt(LEFT) == CubeID::UNDEFINED &&
            hood.neighborAt(RIGHT) != CubeID::UNDEFINED);
}

AnimType CubeStateMachine::getNextAnim(CubeAnim cubeAnim)
{
    switch (mAnimTypes[(int) cubeAnim])
    {
    default:
        return (cubeAnim == CubeAnim_Main) ? AnimType_NotWord : mAnimTypes[(int) cubeAnim];

    case AnimType_HintSlideL:
    case AnimType_HintSlideR:
        return AnimType_HintBarIdle;// TODO HintBarDisappear;

    case AnimType_HintWindUpSlide:
        {
            unsigned nextStartIndex;
            unsigned tiltDir = 0;
            if (calcHintTiltDirection(nextStartIndex, tiltDir))
            {
                switch (tiltDir)
                {
                default:
                    return AnimType_HintBarIdle;

                case 1:
                    return AnimType_HintSlideR;

                case 2:
                    return AnimType_HintSlideL;
                }
            }
        }
        return AnimType_HintBarIdle;

    case AnimType_NormalTilesEnter:
        return AnimType_NotWord;

    case AnimType_NormalTilesExit:
        return AnimType_MetaTilesEnter;

    case AnimType_MetaTilesEnter:
        return AnimType_MetaTilesReveal;

    case AnimType_MetaTilesShow:
    case AnimType_MetaTilesReveal:
        return Dictionary::currentIsMetaPuzzle() ? AnimType_NotWord : AnimType_MetaTilesExit;

    case AnimType_MetaTilesExit:
        return AnimType_NormalTilesEnter;        // TODO other transition at end of city


    case AnimType_NormalTilesReveal:
        return AnimType_NormalTilesExit;

    case AnimType_NewWord:
        if (GameStateMachine::getNumAnagramsLeft() <= 0)
        {
            return AnimType_NormalTilesReveal;
        }
        else
        {
            bool isOldWord = false;
            if (canBeginWord())
            {
                char wordBuffer[MAX_LETTERS_PER_WORD + 1];
                bool isBonus;
                if (beginsWord(isOldWord, wordBuffer, isBonus))
                {

                    if (isOldWord)
                    {
                        return AnimType_OldWord;
                    }
                    else
                    {
                        return AnimType_NewWord;
                    }
                }
                else
                {
                    return AnimType_NotWord;
                }
            }
            else if (hasNoNeighbors())
            {
                return AnimType_NotWord;
            }
            else
            {
                return AnimType_OldWord;
            }
        }
        return AnimType_OldWord;
    }
}

bool CubeStateMachine::beginsWord(bool& isOld, char* wordBuffer, bool& isBonus)
{
    if (canBeginWord())
    {        
        wordBuffer[0] = '\0';
        CubeStateMachine* csm = this;
        bool neighborLetters = false;
        CubeID rightCube = getCube();

        // since this is the left-most cube in a neighbor chain, we can iterate
        // through all the neighbors to the right
        do
        {
            // if there are no letters in the current cube state machine, done
            if (csm->mPuzzleLettersPerCube <= 0)
            {
                break;
            }

            // if fails to get letters in the current cube state machine, done
            char str[MAX_LETTERS_PER_CUBE + 1];
            if (!csm->getLetters(str, false))
            {
                return false;
            }

            // otherwise concat letters from current CubeStateMachine on string
            _SYS_strlcat(wordBuffer, str, GameStateMachine::getCurrentMaxLettersPerWord() + 1);
            neighborLetters = true;

            Neighborhood hood(csm->getCube());
            rightCube = hood.neighborAt(RIGHT);
            csm = GameStateMachine::findCSMFromID(rightCube);
        }
        while (csm != NULL);

        // if a neighbor to the right has letters
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
    for (int side = LEFT; side <= RIGHT; side += 2)
    {
        CubeStateMachine* csm = this;
        CubeID nextCube = CubeID::UNDEFINED;
        // go through all neighbors in the direction of the current side
        do
        {
            if (csm != this)
            {
                ++result;
            }
            Neighborhood hood(csm->getCube());
            nextCube = hood.neighborAt((Side)side);
            csm = GameStateMachine::findCSMFromID(nextCube);
        }
        while(csm != NULL);
    }

    return result;
}

bool CubeStateMachine::hasNoNeighbors()
{
    Neighborhood hood(getCube());
    for (int side = 0; side < NUM_SIDES; ++side)
    {
        if (hood.hasNeighborAt((Side)side))
        {
            return false;
        }
    }
    return true;
}

bool CubeStateMachine::isConnectedToCubeOnSide(CubeID cubeIDStart,
                                               Side side)
{
    if (getCube() == cubeIDStart)
    {
        return true;
    }

    CubeStateMachine* csm = this;

    // search for a matching cubeID in the given direction through the
    // neighbor chain
    CubeID aCube = getCube();
    while (aCube != cubeIDStart)
    {
        Neighborhood hood(aCube);
        aCube = hood.neighborAt(side);
    }
    return (aCube == cubeIDStart);
}

unsigned CubeStateMachine::getNumStates() const
{
    return CubeStateIndex_NumStates;
}

void CubeStateMachine::update(float dt)
{
    mIdleTime += dt;
    for (unsigned i = 0; i < NumCubeAnims; ++i)
    {
        if (mAnimTypes[i] != AnimType_None)
        {
            mAnimTimes[i] += dt;
        }
    }
    updateSpriteParams(dt);
    bool touching = getCube().isTouching();
    if (mTouchHoldWaitForUntouch)
    {
        if (!touching)
        {
            mTouchHoldWaitForUntouch = false;
            mTouchHoldTime = -1.f;
        }
    }
    else if (mTouchHoldTime >= 0.f && touching)
    {
        mTouchHoldTime += dt;
        if (mTouchHoldTime >= 0.5f)
        {
            EventData data;
            data.mInput.mCubeID = getCube();
            WordGame::onEvent(EventID_TouchAndHold, data);
            mTouchHoldWaitForUntouch = true;
        }
    }

    StateMachine::update(dt);
    if ((int)mBG0Panning != (int)mBG0TargetPanning)
    {
        mBG0Panning += (mBG0TargetPanning - mBG0Panning) * dt * 7.5f;
        setPanning(mBG0Panning);
    }
    switch (mAnimTypes[CubeAnim_Main])
    {
    default:
        break;
    }
}

void CubeStateMachine::setPanning(float panning)
{
    mBG0Panning = panning;
    int tilePanning = fmod(mBG0Panning, 144.f);
    tilePanning /= 8;
    // TODO clean this up
    int tileWidth = 12/GameStateMachine::getCurrentMaxLettersPerCube();
    for (unsigned i = 0; i < GameStateMachine::getCurrentMaxLettersPerCube(); ++i)
    {
        mTilePositions[i].x = 2 + tilePanning + i * tileWidth;
        mTilePositions[i].x = ((mTilePositions[i].x + tileWidth + 2) % (16 + 2 * tileWidth));
        mTilePositions[i].x -= tileWidth + 2;
    }
    //mVidBuf->BG0_setPanning(vec((int)mBG0Panning, 0.f));
}

void CubeStateMachine::paint()
{
    if (mPainting)
    {
        return;
    }

    mPainting = true;
    CubeID c = getCube();
    TileBuffer<16,16,1> bg1TileBuf(c);
    ASSERT(mVidBuf != NULL);

    bg1TileBuf.erase(transparent);

    switch (getCurrentStateIndex())
    {
    case CubeStateIndex_Title:
        {
            const float ANIM_START_DELAY = 2.f;

            switch (getCube() - CUBE_ID_BASE)
            {
            default:
                mVidBuf->bg0.image(vec(0, 0), StartBG);
                mVidBuf->sprites[0].setImage(StartPrompt);
                mVidBuf->sprites[0].resize(StartPrompt.pixelWidth(),
                                           StartPrompt.pixelHeight());
                {
                    mVidBuf->sprites[0].move(vec(39, 74));
                    mVidBuf->bg1.setPanning(vec((int)mPanning, 0));
                    bg1TileBuf.image(vec(0, 0), StartLid);
                }
                break;

                // TODO high scores
        #if BLAH
            default:
                paintBorder(vid, ImageIndex_Teeth);
                /* TODO load/save
                paintScoreNumbers(vid, vec(3,4), FontSmall, "High Scores");

                for (unsigned i = arraysize(SavedData::sHighScores) - 1;
                     i >= 0;
                     --i)
                {
                    if (SavedData::sHighScores[i] == 0)
                    {
                        break;
                    }
                    char string[17];
                    sprintf(string, "%.5d", SavedData::sHighScores[i]);
                    paintScoreNumbers(vid, vec(5,4 + (arraysize(SavedData::sHighScores) - i) * 2),
                                 FontSmall,
                                 string);
                }
                */
                break;
        #endif
            }
        }
        break;

    case CubeStateIndex_Menu:
        if (getCube() == WordGame::instance()->getMenuCube())
        {
            mPainting = false;
            return;
        }
        else
        {
            mVidBuf->bg0.image(vec(0,0), MenuBlank);
        }
        break;

    case CubeStateIndex_StoryCityProgression:
        mVidBuf->bg0.image(vec(0,0), MenuBlank);
        if (getCube() == WordGame::instance()->getMenuCube())
        {
            /* TODO animtype?
            char slideOffset = 0;
            char slideStart = -10;
            const char clueHeight = 12;
            const float transitionLength = .5f;
            const float endTransitionStart = 2.5f;
            if (mStateTime[CubeAnim_Main] < transitionLength)
            {
                slideOffset =
                        (1.f - mStateTime[CubeAnim_Main] / transitionLength) * slideStart;
            }
            else if (mStateTime[CubeAnim_Main] >= endTransitionStart)
            {
                if (mStateTime[CubeAnim_Main] >= endTransitionStart + transitionLength)
                {
                    //newState
                }
            }
            */
            mVidBuf->bg0.image(vec(2,2), ClueGreece);
            // TODO drive from python script that looks at puzzles
            unsigned char numMetaPuzzles = 4;
            unsigned char metaPuzzleIndexes[16] = { 8, 16, 23, 33 };
            unsigned char startPuzzle = 0;
            const unsigned char MAX_SLAT_WIDTH = 24;
            unsigned char slatWidth = MIN(MAX_SLAT_WIDTH, 96/numMetaPuzzles);
            //const static *AssetImage SLATS[] = { &Slat1, &Slat2, &Slat3 };
            //const *AssetImage slat = SLATS[slatWidth/8 - 1];
            // TODO odd number handling
            for (unsigned char i = 0; i < numMetaPuzzles; ++i)
            {
                if (WordGame::instance()->getSavedData().isPuzzleSolved(metaPuzzleIndexes[i]))
                {
                }
                else
                {
                    mVidBuf->bg0.image(vec(2 + i * slatWidth/8, 2), Slat3);
                }
            }

        }
        break;

    default:
        mVidBuf->bg0.image(vec(0,0), TileBG);
        paintLetters(bg1TileBuf, true);
        mVidBuf->bg0.setPanning(vec(0.f, 0.f));

        break;
    }

    //bg1.Flush(); // TODO only flush if mask has changed recently
    mVidBuf->bg1.maskedImage(bg1TileBuf, transparent);


    mPainting = false;
}

void CubeStateMachine::paintScore(bool animate,
                                   bool reverseAnim,
                                   bool loopAnim,
                                   bool paintTime,
                                   float animStartTime)
{
#if (0)
    return;
    if (GameStateMachine::getCurrentMaxLettersPerCube() > 1)
    {
        paintTime = false;
    }

    if (teethImageIndex == ImageIndex_Teeth && reverseAnim)
    {
        // no blip when reversing teeth anim (use the same anim, but
        // diff transparency data, to save room)
        teethImageIndex = ImageIndex_Teeth_NoBlip;
    }
    const AssetImage* teethImages[NumImageIndexes] =
    {
        &TeethLoopWordTop,     // ImageIndex_Connected,
        &TeethNewWord2,      // ImageIndex_ConnectedWord,
        &TeethLoopWordLeftTop, // ImageIndex_ConnectedLeft,
        &TeethNewWord2Left,  // ImageIndex_ConnectedLeftWord,
        &TeethLoopWordRightTop,// ImageIndex_ConnectedRight,
        &TeethNewWord2Right, // ImageIndex_ConnectedRightWord,
        &TeethLoopNeighboredTop,// ImageIndex_Neighbored,
        &Teeth,             // ImageIndex_Teeth,
        &Teeth,             // ImageIndex_Teeth_NoBlip,
    };

    const AssetImage* teethNumberImages[] =
    {
        &TeethNewWord3,
        &TeethNewWord4,
        &TeethNewWord5,
    };

    const Vec2 TEETH_NUM_POS(4, 6);
    STATIC_ASSERT(arraysize(teethImages) == NumImageIndexes);
    ASSERT(teethImageIndex >= 0);
    ASSERT(teethImageIndex < (ImageIndex)arraysize(teethImages));
    const AssetImage* teeth = teethImages[teethImageIndex];
    const AssetImage* teethNumber = 0;

    unsigned teethNumberIndex = GameStateMachine::getNewWordLength() / GameStateMachine::getCurrentMaxLettersPerCube();
    if (teethNumberIndex > 2)
    {
        teethNumberIndex -= 3;
        switch (teethImageIndex)
        {
        case ImageIndex_ConnectedWord:
        case ImageIndex_ConnectedLeftWord:
        case ImageIndex_ConnectedRightWord:
            teethNumber = teethNumberImages[MIN(((unsigned)arraysize(teethNumberImages) - 1), teethNumberIndex)];
            break;

        default:
            break;
        }
    }
    unsigned frame = 0;
    unsigned secondsLeft = GameStateMachine::getSecondsLeft();

    if (animate)
    {
        float animTime =  (getTime() - animStartTime)
        if (loopAnim)
        {
            animTime = fmodf(animTime, 1.0f);
        }
        else
        {
            animTime = MIN(animTime, 1.f);
        }

        if (reverseAnim)
        {
            animTime = 1.f - animTime;
        }
        frame = (unsigned) (animTime * teeth->frames);
        frame = MIN(frame, teeth->frames - 1);

        //LOG(("shuffle: [c: %d] anim: %d, frame: %d, time: %f\n", getCube(), teethImageIndex, frame, GameStateMachine::getTime()));

    }
    else if (reverseAnim)
    {
        frame = teeth->frames - 1;
    }

    unsigned bg1Tiles = 2;
    for (unsigned int i = 0; i < 16; ++i) // rows
    {
        for (unsigned j=0; j < 16; ++j) // columns
        {

            Vec2 texCoord(j, i);
            switch (teethImageIndex)
            {
            case ImageIndex_Connected:
                if (i >= 14)
                {
                    teeth = &TeethLoopWordBottom;
                    texCoord.y = i - 14;
                }
                break;

            case ImageIndex_Neighbored:
                if (i >= 14)
                {
                    teeth = &TeethLoopNeighboredBottom;
                    texCoord.y = i - 14;
                }
                break;

            case ImageIndex_ConnectedLeft:
                if (i >= 14)
                {
                    teeth = &TeethLoopWordLeftBottom;
                    texCoord.y = i - 14;
                }
                else if (i >= 2)
                {
                    teeth = &TeethLoopWordLeftLeft;
                    texCoord.x = j % teeth->width;
                    texCoord.y = i - 2;
                }
                break;

            case ImageIndex_ConnectedRight:
                if (i >= 14)
                {
                    teeth = &TeethLoopWordRightBottom;
                    texCoord.y = i - 14;
                }
                else if (i >= 2)
                {
                    teeth = &TeethLoopWordRightRight;
                    texCoord.x = j % teeth->width;
                    texCoord.y = i - 2;
                }
                break;

            default:
                break;
            }

            switch (getTransparencyType(teethImageIndex, frame, j, i))
            {
            case TransparencyType_None:
                if (GameStateMachine::getCurrentMaxLettersPerCube() == 1 ||
                    (animate && (teethImageIndex == ImageIndex_ConnectedRightWord ||
                                teethImageIndex == ImageIndex_ConnectedLeftWord ||
                                teethImageIndex == ImageIndex_ConnectedWord ||
                                teethImageIndex == ImageIndex_Teeth ||
                                teethImageIndex == ImageIndex_Teeth_NoBlip)))
                {
                    // paint this opaque tile
                    // paint BG0
                    if (teethNumber &&
                        frame >= 2 && frame - 2 < teethNumber->frames &&
                        j >= ((unsigned) TEETH_NUM_POS.x) &&
                        j < teethNumber->width + ((unsigned) TEETH_NUM_POS.x) &&
                        i >= ((unsigned) TEETH_NUM_POS.y) &&
                        i < teethNumber->height + ((unsigned) TEETH_NUM_POS.y))
                    {
                        mVidBuf->BG0_drawPartialAsset(vec(j, i),
                                                 vec(j - TEETH_NUM_POS.x, i - TEETH_NUM_POS.y),
                                                 vec(1, 1),
                                                 *teethNumber,
                                                 frame - 2);
                    }
                    else
                    {
                        mVidBuf->BG0_drawPartialAsset(vec(j, i), texCoord, vec(1, 1), *teeth, frame);
                    }
                    break;
                }
                // else fall through

            case TransparencyType_Some:
                if (bg1Tiles < 144)
                {
                    if (teethNumber &&
                        frame >= 2 && frame - 2 < teethNumber->frames &&
                        j >= ((unsigned) TEETH_NUM_POS.x) &&
                        j < teethNumber->width + ((unsigned) TEETH_NUM_POS.x) &&
                        i >= ((unsigned) TEETH_NUM_POS.y) &&
                        i < teethNumber->height + ((unsigned) TEETH_NUM_POS.y))
                    {
                        bg1.DrawPartialAsset(vec(j, i),
                                             vec(j - TEETH_NUM_POS.x, i - TEETH_NUM_POS.y),
                                             vec(1, 1),
                                             *teethNumber,
                                             frame - 2);
                    }
                    else
                    {
                        bg1.DrawPartialAsset(vec(j, i), texCoord, vec(1, 1), *teeth, frame);
                    }
                    ++bg1Tiles;
                }
                break;

            default:
                ASSERT(getTransparencyType(teethImageIndex, frame, j, i) == TransparencyType_All);
                break;
            }
        }
    }

    if (paintTime)
    {
        int AnimType = -1;

        switch (secondsLeft)
        {
        case 30:
        case 20:
        case 10:
            AnimType = secondsLeft/10 + 2;
            break;

        case 3:
        case 2:
        case 1:
            AnimType = secondsLeft - 1;
            break;
        }

        // 1, 2, 3, 10, 20, 30
        float animLength[6] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

        // 1, 2, 3, 10, 20, 30
        const AssetImage* lowDigitAnim[6] =
            {&TeethClockPulse1, &TeethClockPulse2, &TeethClockPulse3,
             &TeethClockPulse0, &TeethClockPulse0, &TeethClockPulse0};

        const AssetImage* highDigitAnim[6] =
            {0, 0, 0,
             &TeethClockPulse1, &TeethClockPulse2, &TeethClockPulse3};

        frame = 0;
        float animTime =
                1.f - fmodf(GameStateMachine::getSecondsLeftFloat(), 1.f);
        if (AnimType >= 0 && animTime < animLength[AnimType])
        {
            // normalize
            animTime /= animLength[AnimType];
            animTime = MIN(animTime, 1.f);
            frame = (unsigned) (animTime * lowDigitAnim[AnimType]->frames);
            frame = MIN(frame, lowDigitAnim[AnimType]->frames - 1);

            if (highDigitAnim[AnimType] > 0)
            {
                bg1.DrawAsset(vec(((3 - 2 + 0) * 4 + 1), 14),
                              *highDigitAnim[AnimType],
                              frame);
            }
            bg1.DrawAsset(vec(((3 - 2 + 1) * 4 + 1), 14),
                          *lowDigitAnim[AnimType],
                          frame);
        }
        else
        {
            String<5> string;
            string << secondsLeft;
            unsigned len = string.size();

            for (unsigned i = 0; i < len; ++i)
            {
                frame = string[i] - '0';
                bg1.DrawAsset(vec(((3 - len + i) * 4 + 1), 14),
                              FontTeeth,
                              frame);
            }
        }
    }

    // TODO merge in 2 ltr cube proto code
    if (GameStateMachine::getCurrentMaxLettersPerCube() > 1 &&
        !(animate &&
            (teethImageIndex == ImageIndex_ConnectedRightWord ||
            teethImageIndex == ImageIndex_ConnectedLeftWord ||
            teethImageIndex == ImageIndex_ConnectedWord ||
            teethImageIndex == ImageIndex_Teeth ||
            teethImageIndex == ImageIndex_Teeth_NoBlip)))
    {
        ASSERT(GameStateMachine::getNumAnagramsLeft() < 100);
        unsigned tensDigit = GameStateMachine::getNumAnagramsLeft() / 10;
        if (tensDigit)
        {
            bg1.DrawAsset(vec(7,11), FontSmall, tensDigit);
        }
        bg1.DrawAsset(vec(8,11), FontSmall, GameStateMachine::getNumAnagramsLeft() % 10);

        if (GameStateMachine::getNumBonusAnagramsLeft())
        {
            tensDigit = GameStateMachine::getNumBonusAnagramsLeft() / 10;
            if (tensDigit)
            {
                bg1.DrawAsset(vec(1,11), FontBonus, tensDigit);
            }
            bg1.DrawAsset(vec(2,11), FontBonus, GameStateMachine::getNumBonusAnagramsLeft() % 10);
        }
    }

    bg1.Flush(); // TODO only flush if mask has changed recently
    //WordGame::instance()->setNeedsPaintSync();
#endif // (0)
}

void CubeStateMachine::paintLetters(TileBuffer<16,16,1> &bg1TileBuf,
                                    bool paintSprites)
{
    const static AssetImage* fonts[] =
    {
        &Font1Letter, &Font2Letter, &Font3Letter,
    };
    const AssetImage& font = *fonts[GameStateMachine::getCurrentMaxLettersPerCube() - 1];

    switch (mAnimTypes[CubeAnim_Hint])
    {
    case AnimType_HintWindUpSlide:
    case AnimType_HintSlideL:
    case AnimType_HintSlideR:
//    case AnimType_HintNeighborL:
//    case AnimType_HintNeighborR:
        break;

    default:
        mVidBuf->sprites[0].hide(); // TODO sprite IDs
        break;
    }

    switch (mAnimTypes[CubeAnim_Main])
    {
    case AnimType_NewWord:
        break;

    default:
        for (unsigned i=1; i<8; ++i)
        {
            mVidBuf->sprites[i].hide();
        }
        break;
    }

    CubeID c = getCube();
    AnimParams params;
    getAnimParams(&params);
    updateAnim(bg1TileBuf, &params);
}

void CubeStateMachine::paintScoreNumbers(BG1Mask &bg1,
                                         const Vec2& position_RHS,
                                         const char* string)
{
    Vec2 position(position_RHS);
    const AssetImage& font = FontSmall;

    const unsigned MAX_SCORE_STRLEN = 7;
    const char* MAX_SCORE_STR = "9999999";

    unsigned len = _SYS_strnlen(string, MAX_SCORE_STRLEN + 1);

    if (len > MAX_SCORE_STRLEN)
    {
        string = MAX_SCORE_STR;
        len = MAX_SCORE_STRLEN;
    }
    position.x -= len - 1;

    for (; *string; ++string)
    {
        unsigned index = *string - '0';
        ASSERT(index < font.numFrames());
        position.x++;
        mVidBuf->bg1.image(position, font, index);
    }
}

bool CubeStateMachine::getAnimParams(AnimParams *params)
{
    bool retval = true;
    ASSERT(params);
    CubeID c = getCube();
    params->mLetters[0] = '\0';
    switch (mAnimTypes[CubeAnim_Main])
    {
    case AnimType_NewWord:
    /* TODO remove case AnimType_MetaTilesReveal:
    case AnimType_NormalTilesReveal:
    */
        params->mSpriteParams = &mSpriteParams;
        break;

    default:
        params->mSpriteParams = 0;
        break;
    }

    switch (mAnimTypes[CubeAnim_Main])
    {
    case AnimType_MetaTilesEnter:
    case AnimType_MetaTilesShow:
    case AnimType_MetaTilesExit:
    case AnimType_MetaTilesReveal:
        params->mAllMetaLetters = true;
        params->mLettersPerCube = mMetaLettersPerCube;
        if (!getMetaLetters(params->mLetters, true))
        {
            retval = false;
        }
        break;

    default:
        if (Dictionary::currentIsMetaPuzzle())
        {
            params->mAllMetaLetters = true;
            params->mLettersPerCube = mMetaLettersPerCube;
            if (!getMetaLetters(params->mLetters, true))
            {
                retval = false;
            }
        }
        else
        {
            params->mAllMetaLetters = false;
            params->mLettersPerCube = mPuzzleLettersPerCube;
            if (!getLetters(params->mLetters, true))
            {
                retval = false;
            }
        }
        break;
    }

    Neighborhood hood(getCube());
    params->mLeftNeighbor = (hood.neighborAt(LEFT) != CubeID::UNDEFINED);
    params->mRightNeighbor = (hood.neighborAt(RIGHT) != CubeID::UNDEFINED);
    params->mCubeID = getCube();
    //params->mBonus = GameStateMachine::getCurrentWordIsBonus();
    unsigned mlpc = GameStateMachine::getCurrentMaxLettersPerCube();
    unsigned letter0Index = mPuzzlePieceIndex * mlpc;
    unsigned mli = Dictionary::getCurrentPuzzleMetaLetterIndex();
    params->mMetaLetterIndex = -1;

    switch (mAnimTypes[CubeAnim_Main])
    {
    case AnimType_SlideL:
    case AnimType_SlideR:
        if (mli >= letter0Index && mli < letter0Index + mlpc)
        {
            params->mMetaLetterIndex =
                    (mli - letter0Index) + (mlpc - mLettersStartOld);
            params->mMetaLetterIndex = (params->mMetaLetterIndex % mlpc);
        }
        break;

    case AnimType_MetaTilesReveal:
    case AnimType_MetaTilesEnter:
        {
            unsigned char m = mMetaPieceIndex * mMetaLettersPerCube;
            for (unsigned char i = m; i < m + mMetaLettersPerCube; ++i)
            {
                if (GameStateMachine::getInstance().isMetaLetterIndexUnlockedLast(i))
                {
                    // dest = (src - start) % lpc = (src + (lpc - start)) % lpc
                    params->mMetaLetterIndex =
                            (i - m) + mMetaLettersPerCube - mMetaLettersStart;
                    params->mMetaLetterIndex =
                            (params->mMetaLetterIndex % mMetaLettersPerCube);
                    break;
                }
            }
        }
        break;

    default:
        if (mli >= letter0Index && mli < letter0Index + mlpc)
        {
            params->mMetaLetterIndex =
                (mli - letter0Index) + (mlpc - mLettersStart);
            params->mMetaLetterIndex = (params->mMetaLetterIndex % mlpc);
        }
        break;
    }
    return retval;
}

void CubeStateMachine::setLettersStart(unsigned s)
{
    if (Dictionary::currentIsMetaPuzzle())
    {
        if (s != mMetaLettersStart)
        {
            mMetaLettersStartOld = mMetaLettersStart;
            mMetaLettersStart = s;
            // new state is ready to react to level order change
            WordGame::instance()->onEvent(EventID_LetterOrderChange, EventData());
        }
    }
    else
    {
        if (s != mLettersStart)
        {
            mLettersStartOld = mLettersStart;
            mLettersStart = s;
            // new state is ready to react to level order change
            WordGame::instance()->onEvent(EventID_LetterOrderChange, EventData());
        }
    }
}

bool CubeStateMachine::canStartHint() const
{
    switch (mAnimTypes[CubeAnim_Hint])
    {
    case AnimType_HintWindUpSlide:
    case AnimType_HintSlideL:
    case AnimType_HintSlideR:
//    case AnimType_HintNeighborL:
//    case AnimType_HintNeighborR:
        return false;

    default:
        {
            return true;
        }
    }
}

bool CubeStateMachine::canUseHint() const
{
    unsigned newLettersStart, tiltDirection;
    return calcHintTiltDirection(newLettersStart, tiltDirection);
}

bool CubeStateMachine::calcHintTiltDirection(unsigned &newLettersStart,
                                             unsigned &tiltDirection) const
{
    unsigned maxLetters;
    const char *letters;
    unsigned start;
    if (mHintSolution[0] == '\0')
    {
        // this piece doesn't matter
        return false;
    }

    if (Dictionary::currentIsMetaPuzzle())
    {
        maxLetters = mMetaLettersPerCube;
        letters = mMetaLetters;
        start = mMetaLettersStart;
    }
    else
    {
        maxLetters = mPuzzleLettersPerCube;
        letters = mLetters;
        start = mLettersStart;
    }
    bool allLettersSame = true;

    const char letter = letters[0];
    for (unsigned j=0; j < maxLetters; ++j)
    {
        if (letters[j] != letter)
        {
            allLettersSame = false;
            break;
        }
    }

    if (allLettersSame)
    {
        return false;
    }

    GameStateMachine::sOnEvent(EventID_UpdateHintSolution, EventData());
    // now determine which way to slide, if any, to put in hint configuration
    bool allMatch = true;
    // for all possible values of a letter start offset
    for (newLettersStart=0; newLettersStart < maxLetters; ++newLettersStart)
    {
        allMatch = true;
        // compare all the letters, using the offset
        for (unsigned j=0; j < maxLetters; ++j)
        {
            if (letters[(j + newLettersStart) % maxLetters] !=
                    mHintSolution[j])
            {
                allMatch = false;
                break;
            }
        }
        if (allMatch)
        {
            break;
        }
    }
    ASSERT(allMatch);// there must be a way to get to the hint solution
    tiltDirection = (start + maxLetters - newLettersStart) % maxLetters;
    return allMatch && tiltDirection != 0; // 1: right, 2: left
}


void CubeStateMachine::setState(unsigned newStateIndex, unsigned oldStateIndex)
{
    LOG("CubeStateMachine::setState: %d,\told: %d\tcube %d\n", newStateIndex, oldStateIndex, (PCubeID)getCube());
    StateMachine::setState(newStateIndex, oldStateIndex);
}

