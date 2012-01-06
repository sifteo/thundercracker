#include "ScoredGameState_StartOfRound.h"
#include "ScoredGameState.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include <string.h>
#include "WordGame.h"
#include "assets.gen.h"

unsigned ScoredGameState_StartOfRound::update(float dt, float stateTime)
{

    const float BLIP_TIME = 4.f/7.f * TEETH_ANIM_LENGTH;
    if (stateTime > BLIP_TIME && stateTime - dt <= BLIP_TIME)
    {
        WordGame::playAudio(blip, AudioChannelIndex_Time);
    }

    if (stateTime > TEETH_ANIM_LENGTH)
    {
        WordGame::playAudio(wordplay_music_versus, AudioChannelIndex_Music, LoopRepeat);
        ScoredGameState::createNewAnagram();
        return GameStateIndex_PlayScored;
    }

    return GameStateIndex_StartOfRoundScored;
}

unsigned ScoredGameState_StartOfRound::onEvent(unsigned eventID, const EventData& data)
{
    ScoredGameState::onAudioEvent(eventID, data);
    switch (eventID)
    {
    case EventID_EnterState:
        WordGame::playAudio(teeth_open, AudioChannelIndex_Teeth);
        break;

    default:
        break;
    }
    return GameStateIndex_StartOfRoundScored;
}
