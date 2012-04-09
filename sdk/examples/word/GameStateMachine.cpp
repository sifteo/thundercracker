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
    StateMachine(GameStateIndex_LoadingFinished), mAnagramCooldown(0.f), mTimeLeft(.0f), mScore(0),
    mNumAnagramsLeft(0), mCurrentMaxLettersPerCube(1), mNumHints(0),
    mMetaLetterUnlockedMask(0xffff), mHintCubeIDOnUpdate(CUBE_ID_UNDEFINED)
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
    STATIC_ASSERT(MAX_LETTERS_PER_WORD < 8 * sizeof(unsigned));
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
            initNewMeta();
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

    case EventID_NewMeta:
        mMetaLetterUnlockedMask = 0;
        for (unsigned char i = 0; i < data.mNewMeta.mMaxLettersPerCube * NUM_CUBES; ++i)
        {
            // FIXME leading spaces for metas
            if (data.mNewMeta.mWord[i] == ' ' || data.mNewMeta.mWord[i] == '\0')
            {
                mMetaLetterUnlockedMask |= (1 << i);
            }
        }
        mMetaLetterUnlockedMaskOld = mMetaLetterUnlockedMask;
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
                if (mLevelProgressData.mPuzzleProgress[i] == CheckMarkState_Hidden)
                {
                    // TODO out of range, unexpected
                    DEBUG_LOG(("Warning: attempted to create another word after finding them all\n"));
                    break;
                }
            }

            // just solved a puzzle, unlock meta letter
            if (mNumAnagramsLeft <= 0)
            {
                char metaLetter = Dictionary::getMetaLetter();
                char meta[MAX_LETTERS_PER_WORD + 1];
                mMetaLetterUnlockedMaskOld = mMetaLetterUnlockedMask;
                unsigned char a, b;
                if (Dictionary::getMetaPuzzle(meta, a, b))
                {
                    unsigned char len = _SYS_strnlen(meta, sizeof meta);
                    for (unsigned char i = 0; i < len; ++i)
                    {
                        if (meta[i] == metaLetter &&
                            !(mMetaLetterUnlockedMask & (1 << i)))
                        {
                            mMetaLetterUnlockedMask |= (1 << i);
                            break;
                        }
                    }
                }
            }
        }
        break;

    default:
        break;
    }
    Dictionary::sOnEvent(eventID, data);

    for (unsigned i = 0; i < arraysize(mCubeStateMachines); ++i)
    {
        mCubeStateMachines[i].onEvent(eventID, data);
    }

    unsigned newStateIndex = getCurrentStateIndex();
    switch (getCurrentStateIndex())
    {
    case GameStateIndex_Title:
        switch (eventID)
        {
        case EventID_EnterState:
            WordGame::playAudio(flap_laugh_fireball, AudioChannelIndex_Time);
            WordGame::playAudio(wordplay_music_sayonara, AudioChannelIndex_Music, LoopRepeat);
            break;

        case EventID_Start:
            newStateIndex = GameStateIndex_StoryStartOfRound;
            break;

        default:
            break;
        }
        break;

    case GameStateIndex_StoryStartOfRound:
        onAudioEvent(eventID, data);
        switch (eventID)
        {
        case EventID_EnterState:
            createNewAnagram();
            break;

        case EventID_Update:
            if (getNumCubesInAnim(AnimType_NotWord) >= NUM_CUBES)
            {

                newStateIndex = GameStateIndex_PlayScored;
            }
            break;

        default:
            break;
        }
        break;

    case GameStateIndex_PlayScored:
        onAudioEvent(eventID, data);
        switch (eventID)
        {
        case EventID_EnterState:
            // fall through
        case EventID_NewPuzzle:
            break;

        case EventID_TouchAndHold:
            newStateIndex = GameStateIndex_PauseMenu;
            break;

        case EventID_Update:
            {
                float dt = data.mUpdate.mDT;
                if (mHintCubeIDOnUpdate != CUBE_ID_UNDEFINED)
                {
                    if (GameStateMachine::getInstance().getNumHints() > 0)
                    {
                        EventData data;
                        Dictionary::findNextSolutionWordPieces(NUM_CUBES,
                                                               GameStateMachine::getCurrentMaxLettersPerCube(),
                                                               data.mHintSolutionUpdate.mHintSolution);
                        GameStateMachine::sOnEvent(EventID_HintSolutionUpdated, data);

                        // first make sure a hint isn't already active
                        bool canStartHint = true;
                        bool canUseHint = false;
                        for (unsigned ci = 0; ci < NUM_CUBES; ++ci)
                        {
                            Cube::ID cubeID = ci + CUBE_ID_BASE;
                            CubeStateMachine *csm =
                                GameStateMachine::findCSMFromID(cubeID);
                            ASSERT(csm);
                            if (!csm->canStartHint())
                            {
                                canStartHint = false;
                                break;
                            }

                            if (csm->canUseHint())
                            {
                                canUseHint = true;
                            }
                        }

                        if (canStartHint && canUseHint)
                        {
                            // then start the hint wind-up on all cubes
                            // when the wind-up finishes, the first cube that can show a hint
                            // will do so and message the rest to stop
                            for (unsigned ci = 0; ci < NUM_CUBES; ++ci)
                            {
                                Cube::ID cubeID = ci + CUBE_ID_BASE;
                                CubeStateMachine *csm =
                                    GameStateMachine::findCSMFromID(cubeID);
                                ASSERT(csm);
                                csm->startHint();
                            }
                        }
                    }
                    mHintCubeIDOnUpdate = CUBE_ID_UNDEFINED;
                }

                if (GameStateMachine::getSecondsLeft() <= 0)
                {
                    newStateIndex = GameStateIndex_EndOfRoundScored;
                }
                else
                {
                    if (GameStateMachine::getNumAnagramsLeft() <= 0 &&
                        GameStateMachine::getNumCubesInAnim(AnimType_NewWord) <= 0 &&
                        GameStateMachine::getNumCubesInAnim(AnimType_NormalTilesReveal) <= 0 &&
                        GameStateMachine::getNumCubesInAnim(AnimType_NormalTilesExit) <= 0)
                    {
                        switch (Dictionary::getPuzzleIndex())
                        {
                        default:
                            // wait for all the cube states to exit the new word state
                            // then shuffle
                            createNewAnagram();
                            WordGame::playAudio(shake, AudioChannelIndex_Shake);
                            // TODO new audio
                            break;
                        }
                    }
                }
            }
            break;

        case EventID_NewWordFound:
            {
                // count total hints and add one
                GameStateMachine::getInstance().setNumHints(MIN(GameStateMachine::getInstance().getNumHints() + 1, MAX_HINTS));
            }
    #if (0)
    #ifndef SIFTEO_SIMULATOR
            // skip to next puzzle
            if (GameStateMachine::getAnagramCooldown() <= .0f &&
                GameStateMachine::getSecondsLeft() > 3)
            {
                WordGame::playAudio(shake, AudioChannelIndex_Shake);
                newStateIndex = GameStateIndex_ShuffleScored;
            }
    #endif
    #endif
            break;

        case EventID_UpdateHintSolution:
            {
                EventData data;
                Dictionary::findNextSolutionWordPieces(NUM_CUBES,
                                                       GameStateMachine::getCurrentMaxLettersPerCube(),
                                                       data.mHintSolutionUpdate.mHintSolution);
                GameStateMachine::sOnEvent(EventID_HintSolutionUpdated, data);
            }
            break;

        case EventID_WordBroken:
            mHintCubeIDOnUpdate = data.mWordBroken.mCubeIDStart; // wait until next update, so all events can resolve first
            break;

        case EventID_SpendHint:
            GameStateMachine::getInstance().setNumHints(MAX(GameStateMachine::getInstance().getNumHints() - 1, 0));
            break;

        default:
            break;
        }
        break;

    case GameStateIndex_StartOfRoundScored:
        onAudioEvent(eventID, data);
        switch (eventID)
        {
        case EventID_EnterState:
            WordGame::playAudio(teeth_open, AudioChannelIndex_Teeth);
            break;

        case EventID_Update:
            {
                float dt = data.mUpdate.mDT;
                const float BLIP_TIME = 4.f/7.f * 1.5f;
                if (mStateTime > BLIP_TIME && mStateTime - dt <= BLIP_TIME)
                {
                    WordGame::playAudio(blip, AudioChannelIndex_Time);
                }

                if (mStateTime > TRANSITION_ANIM_LENGTH)
                {
                    WordGame::playAudio(wordplay_music_versus, AudioChannelIndex_Music, LoopRepeat);
                    createNewAnagram();
                    newStateIndex = GameStateIndex_PlayScored;
                }
            }
            break;

        default:
            break;
        }
        break;

    case GameStateIndex_EndOfRoundScored:
        onAudioEvent(eventID, data);
        switch (eventID)
        {
        case EventID_EnterState:
            WordGame::playAudio(wordplay_music_sayonara, AudioChannelIndex_Music, LoopRepeat);
            WordGame::playAudio(teeth_close, AudioChannelIndex_Teeth);
            break;

        case EventID_Shake:
            if (GameStateMachine::getTime() > TRANSITION_ANIM_LENGTH)
            {
                WordGame::playAudio(shake, AudioChannelIndex_Shake);
                newStateIndex = GameStateIndex_StartOfRoundScored;
            }
            break;

        default:
            break;
        }
        break;

    case GameStateIndex_ShuffleScored:
        onAudioEvent(eventID, data);
        switch (eventID)
        {
        case EventID_EnterState:
            mNeedsNewAnagram = true;
            WordGame::playAudio(teeth_close, AudioChannelIndex_Teeth);
            break;

        case EventID_Update:
            {
                float dt = data.mUpdate.mDT;
                const float BLIP_TIME = 4.f/7.f * TRANSITION_ANIM_LENGTH + TRANSITION_ANIM_LENGTH;
                if (mStateTime > TRANSITION_ANIM_LENGTH && mNeedsNewAnagram)
                {
                    createNewAnagram();
                    WordGame::playAudio(teeth_open, AudioChannelIndex_Teeth);
                    mNeedsNewAnagram = false;
                }
                else if (mStateTime > BLIP_TIME && mStateTime - dt <= BLIP_TIME)
                {
                    WordGame::playAudio(blip, AudioChannelIndex_Time);
                }

                newStateIndex =
                        (mStateTime > TRANSITION_ANIM_LENGTH * 2.f) ? GameStateIndex_PlayScored : GameStateIndex_ShuffleScored;
            }
            break;

        default:
            break;
        }
        break;

    case GameStateIndex_StoryCityProgression:
        switch (eventID)
        {
        case EventID_Shake:
            WordGame::playAudio(shake, AudioChannelIndex_Shake);
            newStateIndex = GameStateIndex_ShuffleScored;
        }
        break;

    case GameStateIndex_Loading:
        // TODO remove or use
        break;

    case GameStateIndex_LoadingFinished:
        switch (eventID)
        {
        case EventID_Update:
            if (WordGame::instance()->getSavedData().isFirstRun())
            {
                newStateIndex = GameStateIndex_Title;
            }
            else
            {
                newStateIndex = GameStateIndex_MainMenu;
            }
            break;

        default:
            break;
        }
        break;

    case GameStateIndex_MainMenu:
    case GameStateIndex_PauseMenu:
        switch (eventID)
        {
        case EventID_EnterState:
            WordGame::instance()->setNeedsPaintSync();
            break;

        case EventID_Update:
            {
                struct MenuEvent e;
                Menu &m = *WordGame::instance()->getMenu(); // TODO get pause menu
                m.reset();
                bool exitMenu = false;
                while (m.pollEvent(&e) && !exitMenu)
                {
                    switch(e.type)
                    {
                    case MENU_ITEM_PRESS:
                        // TODO update game state before continuing, depending on selection
                        if (e.item == 0)
                        {
                            newStateIndex =
                                    (getCurrentStateIndex() == GameStateIndex_MainMenu) ?
                                        GameStateIndex_StoryStartOfRound :
                                        GameStateIndex_PlayScored;
                            exitMenu = true;
                            {
                                EventData data;
                                data.mTouchAndHoldWaitForUntouch.mCubeID = WordGame::instance()->getMenuCube()->id();
                                WordGame::onEvent(EventID_TouchAndHoldWaitForUntouch, data);
                            }
                        }
                        else
                        {
                            m.preventDefault();
                        }
                        break;

                    case MENU_EXIT:
                        // this is not possible when pollEvent is used as the condition to the while loop.
                        // NOTE: this event should never have its default handler skipped.
                        ASSERT(false);
                        break;

                    case MENU_NEIGHBOR_ADD:
                        //DEBUG_LOG(("found cube %d on side %d of menu (neighbor's %d side)\n",
                          //   e.neighbor.neighbor, e.neighbor.masterSide, e.neighbor.neighborSide));
                        break;
                    case MENU_NEIGHBOR_REMOVE:
                        //DEBUG_LOG(("lost cube %d on side %d of menu (neighbor's %d side)\n",
                          //   e.neighbor.neighbor, e.neighbor.masterSide, e.neighbor.neighborSide));
                        break;

                    case MENU_ITEM_ARRIVE:
                        //DEBUG_LOG(("arriving at menu item %d\n", e.item));
                        break;

                    case MENU_ITEM_DEPART:
                        //DEBUG_LOG(("departing from menu item %d\n", e.item));
                        break;

                    case MENU_PREPAINT:
                        // if you are drawing/animating the other cubes, do your work here
                    // do your implementation-specific drawing here
                        // NOTE: this event should never have its default handler skipped.
                        WordGame::onEvent(EventID_Paint, EventData());
                        break;

                    case MENU_UNEVENTFUL:
                        // this should never happen. if it does, it can/should be ignored.
                        ASSERT(false);
                        break;
                    }
                }
                //ASSERT(e.type == MENU_EXIT);
                //DEBUG_LOG(("Selected Game: %d\n", e.item));

            }
            break;
        }
        break;

    case GameStateIndex_CutScene:
        break;

    case GameStateIndex_CubeBuddyUnlock:
        break;

    default:
        ASSERT(0); // unexpected state value
        break;
    }

    if (newStateIndex != getCurrentStateIndex())
    {
        ASSERT(eventID != EventID_EnterState && eventID != EventID_ExitState); // don't change states while changing states
        setState(newStateIndex, getCurrentStateIndex());
    }

    return newStateIndex;
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

void GameStateMachine::setState(unsigned newStateIndex, unsigned oldStateIndex)
{
    EventData data;
    data.mGameStateChanged.mPreviousStateIndex = getCurrentStateIndex();
    DEBUG_LOG(("GameStateMachine::setState: %d,\told: %d\n", newStateIndex, data.mGameStateChanged.mPreviousStateIndex));
    data.mGameStateChanged.mNewStateIndex = newStateIndex;
    StateMachine::setState(newStateIndex, oldStateIndex);
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

void GameStateMachine::initNewMeta()
{
    EventData newMeta;
    char metaNoSpaces[MAX_LETTERS_PER_WORD + 1];
    if (Dictionary::getMetaPuzzle(metaNoSpaces,
                                  newMeta.mNewMeta.mLeadingSpaces,
                                  newMeta.mNewMeta.mMaxLettersPerCube))
    {
        // add spaces
        _SYS_memset8((uint8_t*)newMeta.mNewMeta.mWord, ' ', sizeof(newMeta.mNewMeta.mWord));
        unsigned len = _SYS_strnlen(metaNoSpaces, sizeof metaNoSpaces);
        for (unsigned char i = 0; i < len; ++i)
        {
            newMeta.mNewMeta.mWord[i + newMeta.mNewMeta.mLeadingSpaces] =
                    metaNoSpaces[i];
        }
        newMeta.mNewMeta.mWord[newMeta.mNewMeta.mMaxLettersPerCube * NUM_CUBES] = '\0';

        WordGame::getRandomCubePermutation(newMeta.mNewMeta.mCubeOrderingIndexes);
        // for each cube, choose a random letter shift
        for (unsigned ci = 0; ci < NUM_CUBES; ++ci)
        {
            newMeta.mNewMeta.mLetterStartIndexes[ci] =
                    2;//WordGame::random.randrange(newMeta.mNewMeta.mMaxLettersPerCube);
        }
        onEvent(EventID_NewMeta, newMeta);
    }

}

void GameStateMachine::onAudioEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_AddNeighbor:
        WordGame::playAudio(neighbor, AudioChannelIndex_Neighbor, LoopOnce, AudioPriority_Low);
        break;

    case EventID_NewWordFound:
//        WordGame::playAudio(fireball_laugh, AudioChannelIndex_Score);
        switch (_SYS_strnlen(data.mWordFound.mWord, GameStateMachine::getCurrentMaxLettersPerWord()) / GameStateMachine::getCurrentMaxLettersPerCube())
        {
        case 2:
            WordGame::playAudio(fanfare_fire_laugh_01, AudioChannelIndex_Score);
            break;

        case 3:
            WordGame::playAudio(fanfare_fire_laugh_02, AudioChannelIndex_Score);
            break;

        case 4:
            WordGame::playAudio(fanfare_fire_laugh_03, AudioChannelIndex_Score);
            break;

        case 5:
            WordGame::playAudio(fanfare_fire_laugh_04, AudioChannelIndex_Score);
            break;

        default:
            WordGame::playAudio(fanfare_fire_laugh_04, AudioChannelIndex_Score);
            break;
        }
        break;

    case EventID_OldWordFound:
        WordGame::playAudio(lip_snort, AudioChannelIndex_Score, LoopOnce, AudioPriority_Low);
        break;

    case EventID_ClockTick:
        switch (GameStateMachine::getSecondsLeft())
        {
        case 30:
//            WordGame::playAudio(bonus, AudioChannelIndex_Time);

            WordGame::playAudio(timer_30sec, AudioChannelIndex_Time, LoopOnce, AudioPriority_High);
            break;

        case 20:
//            WordGame::playAudio(pause_on, AudioChannelIndex_Time);
            WordGame::playAudio(timer_20sec, AudioChannelIndex_Time, LoopOnce, AudioPriority_High);
            break;

        case 10:
//            WordGame::playAudio(pause_off, AudioChannelIndex_Time);
            WordGame::playAudio(timer_10sec, AudioChannelIndex_Time, LoopOnce, AudioPriority_High);
            break;

        case 3:
            WordGame::playAudio(timer_3sec, AudioChannelIndex_Time, LoopOnce, AudioPriority_High);
            break;

        case 2:
            WordGame::playAudio(timer_2sec, AudioChannelIndex_Time, LoopOnce, AudioPriority_High);
            break;

        case 1:
            WordGame::playAudio(timer_1sec, AudioChannelIndex_Time, LoopOnce, AudioPriority_High);
            break;
        }
        break;

    default:
        break;
    }
}

void GameStateMachine::createNewAnagram()
{
    // make a new anagram of letters halfway through the shuffle animation
    EventData data;
    Dictionary::getNextPuzzle(data.mNewPuzzle.mWord,
                         data.mNewPuzzle.mNumAnagrams,
                         data.mNewPuzzle.mLeadingSpaces,
                         data.mNewPuzzle.mMaxLettersPerCube);

            //(unsigned)_ceilf(_SYS_strnlen(data.mNewPuzzle.mWord, MAX_LETTERS_PER_WORD + 1) / 3.f));

    // add any leading and/or trailing spaces to odd-length words
    char spacesAdded[MAX_LETTERS_PER_WORD + 1];
    _SYS_memset8((uint8_t*)spacesAdded, 0, sizeof(spacesAdded));

    unsigned wordLen = _SYS_strnlen(data.mNewPuzzle.mWord, MAX_LETTERS_PER_WORD + 1);

    // TODO data-driven
    GameStateMachine::setCurrentMaxLettersPerCube(wordLen > 6 ? 3 : 2);

    ASSERT(GameStateMachine::getCurrentMaxLettersPerWord() >= wordLen);
    unsigned numBlanks =
            GameStateMachine::getCurrentMaxLettersPerWord() - wordLen;
    unsigned leadingBlanks = 0;
    if (numBlanks == 0)
    {
        _SYS_strlcpy(spacesAdded, data.mNewPuzzle.mWord, sizeof spacesAdded);
    }
    else
    {
        if (data.mNewPuzzle.mLeadingSpaces)
        {
            leadingBlanks = numBlanks;
        }
        else
        {
            leadingBlanks = 0;
        }
        // TODO no puzzles currently have leading blanks, but the offline
        // dict tool is aware of the alternate
        //WordGame::random.randrange(numBlanks + 1);

        unsigned i;
        for (i = 0;
             i < GameStateMachine::getCurrentMaxLettersPerWord();
             ++i)
        {
            if (i < leadingBlanks || i >= leadingBlanks + wordLen)
            {
                spacesAdded[i] = ' ';
            }
            else
            {
                spacesAdded[i] = data.mNewPuzzle.mWord[i - leadingBlanks];
            }
        }
        spacesAdded[i] = '\0';
    }

    char scrambled[MAX_LETTERS_PER_WORD + 1];
    // TODO data-driven, scramble or not
    if (!Dictionary::shouldScrambleCurrentWord())
    {
        // don't scramble
        _SYS_strlcpy(scrambled, spacesAdded, sizeof scrambled);
        for (int i = 0; i < (int)arraysize(data.mNewPuzzle.mCubeOrderingIndexes); ++i)
        {
            data.mNewPuzzle.mCubeOrderingIndexes[i] = i;
            data.mNewPuzzle.mLetterStartIndexes[i] = 0;
        }
    }
    else
    {

        switch (GameStateMachine::getCurrentMaxLettersPerCube())
        {
        case 1:
            _SYS_memset8((uint8_t*)scrambled, 0, sizeof(scrambled));
            for (unsigned i = 0; i < GameStateMachine::getCurrentMaxLettersPerWord(); ++i)
            {
                if (spacesAdded[i] == '\0')
                {
                    break;
                }

                // for each letter, place it randomly in the scrambled array
                for (unsigned j = WordGame::random.randrange(GameStateMachine::getCurrentMaxLettersPerWord());
                     true;
                     j = (j + 1) % GameStateMachine::getCurrentMaxLettersPerWord())
                {
                    if (scrambled[j] == '\0')
                    {
                        scrambled[j] = spacesAdded[i];
                        break;
                    }
                }
            }
            break;

        default:
            // scramble the string by randomly permuting the puzzle pieces
            // (sets of letters associated with cubes), and then randomly shift
            // the order of each of those sets (a linear shift of position, which
            // wraps around)

            _SYS_strlcpy(scrambled, spacesAdded, sizeof scrambled);
            WordGame::getRandomCubePermutation(data.mNewPuzzle.mCubeOrderingIndexes);
            // for each cube, choose a random letter shift
            for (unsigned ci = 0; ci < NUM_CUBES; ++ci)
            {
                data.mNewPuzzle.mLetterStartIndexes[ci] =
                        WordGame::random.randrange(GameStateMachine::getCurrentMaxLettersPerCube());
            }
            break;
        }
    }

    LOG(("scrambled %s to %s\n", spacesAdded, scrambled));
    ASSERT(_SYS_strnlen(scrambled, GameStateMachine::getCurrentMaxLettersPerWord() + 1) ==
           _SYS_strnlen(spacesAdded, GameStateMachine::getCurrentMaxLettersPerWord() + 1));
    _SYS_strlcpy(data.mNewPuzzle.mWord, scrambled, sizeof data.mNewPuzzle.mWord);
    wordLen = _SYS_strnlen(data.mNewPuzzle.mWord, sizeof data.mNewPuzzle.mWord);
    GameStateMachine::sOnEvent(EventID_NewPuzzle, data);

   if (Dictionary::currentStartsNewMetaPuzzle())
    {
       GameStateMachine::getInstance().initNewMeta();
    }
}

