#include "ScoredGameState.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include "assets.gen.h"
#include "WordGame.h"
#include <string.h>

unsigned ScoredGameState::update(float dt, float stateTime)
{
    return (GameStateMachine::getSecondsLeft() <= 0) ?
                GameStateIndex_EndOfRoundScored : GameStateIndex_PlayScored;
}

unsigned ScoredGameState::onEvent(unsigned eventID, const EventData& data)
{
    onAudioEvent(eventID, data);
    switch (eventID)
    {
    case EventID_Input:
        if (GameStateMachine::getAnagramCooldown() <= .0f &&
            GameStateMachine::getSecondsLeft() > 3)
        {
            WordGame::playAudio(shake, AudioChannelIndex_Shake);
            return GameStateIndex_ShuffleScored;
        }
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
        WordGame::playAudio(neighbor, AudioChannelIndex_Neighbor);
        break;

    case EventID_NewWordFound:
//        WordGame::playAudio(fireball_laugh, AudioChannelIndex_Score);
        switch (strlen(data.mWordFound.mWord))
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
        WordGame::playAudio(lip_snort, AudioChannelIndex_Score);
        break;

    case EventID_GameStateChanged:
        switch (data.mGameStateChanged.mNewStateIndex)
        {
        case GameStateIndex_EndOfRoundScored:
            // TODO diff endings
            WordGame::playAudio(timeup_01, AudioChannelIndex_Time);
            break;
        }
        break;

    case EventID_ClockTick:
        switch (GameStateMachine::getSecondsLeft())
        {
        case 30:
//            WordGame::playAudio(bonus, AudioChannelIndex_Time);

            WordGame::playAudio(timer_30sec, AudioChannelIndex_Time);
            break;

        case 20:
//            WordGame::playAudio(pause_on, AudioChannelIndex_Time);
            WordGame::playAudio(timer_20sec, AudioChannelIndex_Time);
            break;

        case 10:
//            WordGame::playAudio(pause_off, AudioChannelIndex_Time);
            WordGame::playAudio(timer_10sec, AudioChannelIndex_Time);
            break;

        case 3:
            WordGame::playAudio(timer_3sec, AudioChannelIndex_Time);
            break;

        case 2:
            WordGame::playAudio(timer_2sec, AudioChannelIndex_Time);
            break;

        case 1:
            WordGame::playAudio(timer_1sec, AudioChannelIndex_Time);
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
    Dictionary::pickWord(data.mNewAnagram.mWord);

    // scramble the string (random permutation)
    // TODO multiple letters per cube
    char scrambled[MAX_LETTERS_PER_WORD + 1];
    memset(scrambled, 0, sizeof(scrambled));
    for (unsigned i = 0; i < MAX_LETTERS_PER_WORD; ++i)
    {
        // for each letter, place it randomly in the scrambled array
        for (unsigned j = WordGame::rand(MAX_LETTERS_PER_WORD);
             true;
             j = (j + 1) % MAX_LETTERS_PER_WORD)
        {
            if (scrambled[j] == '\0')
            {
                scrambled[j] = data.mNewAnagram.mWord[i];
                break;
            }
        }
    }

    LOG(("scrambled %s to %s\n", data.mNewAnagram.mWord, scrambled));
    ASSERT(strlen(scrambled) == MAX_LETTERS_PER_WORD);
    strcpy(data.mNewAnagram.mWord, scrambled);
    unsigned wordLen = strlen(data.mNewAnagram.mWord);
    unsigned numCubes = GameStateMachine::GetNumCubes();
    if ((wordLen % numCubes) == 0)
    {
        // all cubes have word fragments of the same length
        data.mNewAnagram.mOffLengthIndex = -1;
    }
    else
    {
        // pick a cube index randomly to have the off length
        // word fragment
        //data.mNewAnagram.mOddIndex = ;
    }
    GameStateMachine::sOnEvent(EventID_NewAnagram, data);
}
