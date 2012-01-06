#include "ScoredGameState_Shuffle.h"
#include "ScoredGameState.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include <string.h>
#include "WordGame.h"
#include "assets.gen.h"

unsigned ScoredGameState_Shuffle::update(float dt, float stateTime)
{
    const float BLIP_TIME = 4.f/7.f * TEETH_ANIM_LENGTH + TEETH_ANIM_LENGTH;
    if (stateTime > TEETH_ANIM_LENGTH && stateTime - dt <= TEETH_ANIM_LENGTH)
    {
        ScoredGameState::createNewAnagram();
        WordGame::playAudio(teeth_open, AudioChannelIndex_Teeth);
    }
    else if (stateTime > BLIP_TIME && stateTime - dt <= BLIP_TIME)
    {
        WordGame::playAudio(blip, AudioChannelIndex_Time);
    }

    return (stateTime > TEETH_ANIM_LENGTH * 2.f) ? GameStateIndex_PlayScored : GameStateIndex_ShuffleScored;
}

unsigned ScoredGameState_Shuffle::onEvent(unsigned eventID, const EventData& data)
{
    ScoredGameState::onAudioEvent(eventID, data);
    switch (eventID)
    {
    case EventID_EnterState:
        WordGame::playAudio(teeth_close, AudioChannelIndex_Teeth);
        break;

    default:
        break;
    }
    return GameStateIndex_ShuffleScored;
}
