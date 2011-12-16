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
    switch (eventID)
    {
    case EventID_EnterState:
        GameStateMachine::sOnEvent(EventID_NewRound, EventData());
        WordGame::playAudio(wordplay_music_versus, AudioChannelIndex_Music, LoopRepeat);
        // fall through
    case EventID_Input:
        if (GameStateMachine::getAnagramCooldown() <= .0f)
        {
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
        break;

    case EventID_AddNeighbor:
        WordGame::playAudio(clink, AudioChannelIndex_Neighbor);
        break;

    case EventID_NewWordFound:
        WordGame::playAudio(word, AudioChannelIndex_Score);
        break;

    case EventID_OldWordFound:
        WordGame::playAudio(oldword, AudioChannelIndex_Score);
        break;

    case EventID_NewAnagram:
        WordGame::playAudio(shuffle, AudioChannelIndex_NewAnagram);
        break;

    case EventID_EndRound:
        WordGame::playAudio(timesup, AudioChannelIndex_Time);
        break;

    case EventID_ClockTick:
        switch (GameStateMachine::getSecondsLeft())
        {
        case 30:
            WordGame::playAudio(seconds30, AudioChannelIndex_Time);
            break;
        case 20:
            WordGame::playAudio(seconds20, AudioChannelIndex_Time);
            break;
        case 10:
            WordGame::playAudio(seconds10, AudioChannelIndex_Time);
            break;
        case 3:
            WordGame::playAudio(time03, AudioChannelIndex_Time);
            break;
        case 2:
            WordGame::playAudio(time02, AudioChannelIndex_Time);
            break;
        case 1:
            WordGame::playAudio(time01, AudioChannelIndex_Time);
            break;
        }
        break;

    default:
        break;
    }
    return GameStateIndex_PlayScored;
}


