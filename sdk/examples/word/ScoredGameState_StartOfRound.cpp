#include "ScoredGameState_StartOfRound.h"
#include "ScoredGameState.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include <string.h>
#include "WordGame.h"
#include "audio.gen.h"

unsigned ScoredGameState_StartOfRound::update(float dt, float stateTime)
{

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
    switch (eventID)
    {
    case EventID_EnterState:
        break;

    default:
        break;
    }
    return GameStateIndex_StartOfRoundScored;
}
