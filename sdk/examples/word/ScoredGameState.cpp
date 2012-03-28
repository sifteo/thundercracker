#include "ScoredGameState.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include "assets.gen.h"
#include "WordGame.h"
#include "Utility.h"

ScoredGameState::ScoredGameState() : mHintCubeIDOnUpdate(CUBE_ID_UNDEFINED) {}

unsigned ScoredGameState::update(float dt, float stateTime)
{
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
        return GameStateIndex_EndOfRoundScored;
    }
    else
    {
        if (GameStateMachine::getNumAnagramsLeft() <= 0 &&
            GameStateMachine::getNumCubesInAnim(AnimType_NewWord) <= 0 &&
            GameStateMachine::getNumCubesInAnim(AnimType_NormalTilesExit) <= 0)
        {
            switch (Dictionary::getPuzzleIndex())
            {
            case 6:  // acre
            case 12: // part
            case 18: // career
            case 24: // begun
                // TODO detect end of level through data
                return GameStateIndex_StoryCityProgression;
            default:
                // wait for all the cube states to exit the new word state
                // then shuffle
                ScoredGameState::createNewAnagram();
                WordGame::playAudio(shake, AudioChannelIndex_Shake);
                // TODO new audio
                return GameStateIndex_PlayScored;
            }
        }

        return GameStateIndex_PlayScored;
    }
}


unsigned ScoredGameState::onEvent(unsigned eventID, const EventData& data)
{
    onAudioEvent(eventID, data);
    switch (eventID)
    {
    // TODO put in the pause menu on touch
    case EventID_EnterState:
        ScoredGameState::createNewAnagram();
        // fall through
    case EventID_NewAnagram:
        // TODO reset hints,

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
            return GameStateIndex_ShuffleScored;
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
    return GameStateIndex_PlayScored;
}

void ScoredGameState::onAudioEvent(unsigned eventID, const EventData& data)
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

void ScoredGameState::createNewAnagram()
{
    // make a new anagram of letters halfway through the shuffle animation
    EventData data;
    Dictionary::pickWord(data.mNewAnagram.mWord,
                         data.mNewAnagram.mNumAnagrams,
                         data.mNewAnagram.mLeadingSpaces,
                         data.mNewAnagram.mMaxLettersPerCube);

            //(unsigned)_ceilf(_SYS_strnlen(data.mNewAnagram.mWord, MAX_LETTERS_PER_WORD + 1) / 3.f));

    // add any leading and/or trailing spaces to odd-length words
    char spacesAdded[MAX_LETTERS_PER_WORD + 1];
    _SYS_memset8((uint8_t*)spacesAdded, 0, sizeof(spacesAdded));

    unsigned wordLen = _SYS_strnlen(data.mNewAnagram.mWord, MAX_LETTERS_PER_WORD + 1);

    // TODO data-driven
    GameStateMachine::setCurrentMaxLettersPerCube(wordLen > 6 ? 3 : 2);

    ASSERT(GameStateMachine::getCurrentMaxLettersPerWord() >= wordLen);
    unsigned numBlanks =
            GameStateMachine::getCurrentMaxLettersPerWord() - wordLen;
    unsigned leadingBlanks = 0;
    if (numBlanks == 0)
    {
        _SYS_strlcpy(spacesAdded, data.mNewAnagram.mWord, sizeof spacesAdded);
    }
    else
    {
        if (data.mNewAnagram.mLeadingSpaces)
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
                spacesAdded[i] = data.mNewAnagram.mWord[i - leadingBlanks];
            }
        }
        spacesAdded[i] = '\0';
    }

    char scrambled[MAX_LETTERS_PER_WORD + 1];
    // TODO data-driven, scramble or not
    if (!Dictionary::doScrambleCurrentWord())
    {
        // don't scramble
        _SYS_strlcpy(scrambled, spacesAdded, sizeof scrambled);
        for (int i = 0; i < (int)arraysize(data.mNewAnagram.mPuzzlePieceIndexes); ++i)
        {
            data.mNewAnagram.mPuzzlePieceIndexes[i] = i;
            data.mNewAnagram.mPuzzleStartIndexes[i] = 0;
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
            {
                // first scramble the cube to word fragments mapping
                int cubeIndexes[NUM_CUBES];
                for (int i = 0; i < (int)arraysize(cubeIndexes); ++i)
                {
                    cubeIndexes[i] = i;
                }

                // assign cube indexes to the puzzle piece indexes array, randomly
                for (int i = 0; i < (int)arraysize(data.mNewAnagram.mPuzzlePieceIndexes); ++i)
                {
                    for (unsigned j = WordGame::random.randrange((unsigned)1, NUM_CUBES);
                         true;
                         j = ((j + 1) % NUM_CUBES))
                    {
                        if (cubeIndexes[j] >= 0)
                        {
                            data.mNewAnagram.mPuzzlePieceIndexes[i] = cubeIndexes[j];
                            cubeIndexes[j] = -1;
                            break;
                        }
                    }
                }

                for (unsigned ci = 0; ci < NUM_CUBES; ++ci)
                {
                    // for each cube, choose a random letter shift
                    data.mNewAnagram.mPuzzleStartIndexes[ci] =
                            WordGame::random.randrange(GameStateMachine::getCurrentMaxLettersPerCube());
                }
            }
            break;
        }
    }

    LOG(("scrambled %s to %s\n", spacesAdded, scrambled));
    ASSERT(_SYS_strnlen(scrambled, GameStateMachine::getCurrentMaxLettersPerWord() + 1) ==
           _SYS_strnlen(spacesAdded, GameStateMachine::getCurrentMaxLettersPerWord() + 1));
    _SYS_strlcpy(data.mNewAnagram.mWord, scrambled, sizeof data.mNewAnagram.mWord);
    wordLen = _SYS_strnlen(data.mNewAnagram.mWord, sizeof data.mNewAnagram.mWord);
    GameStateMachine::sOnEvent(EventID_NewAnagram, data);
}
