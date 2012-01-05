#include "ScoredGameState.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include "audio.gen.h"
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
//        WordGame::playAudio(pause_off, AudioChannelIndex_Neighbor);
        WordGame::playAudio(neighbor, AudioChannelIndex_Neighbor);
        break;

    case EventID_NewWordFound:
//        WordGame::playAudio(fireball_laugh, AudioChannelIndex_Score);
        WordGame::playAudio(fanfare_fire_laugh_01, AudioChannelIndex_Score);
        break;

    case EventID_OldWordFound:
//        WordGame::playAudio(pause_on, AudioChannelIndex_Score);
        WordGame::playAudio(lip_snort, AudioChannelIndex_Score);
        break;

    /*
    case EventID_NewAnagram:
        WordGame::playAudio(swap, AudioChannelIndex_NewAnagram);
        break;
    */

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
            WordGame::playAudio(timer_30sec, AudioChannelIndex_Time);
            break;
        case 20:
            WordGame::playAudio(timer_20sec, AudioChannelIndex_Time);
            break;
        case 10:
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
    data.mNewAnagram.mWord = Dictionary::pickWord(MAX_CUBES);
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
