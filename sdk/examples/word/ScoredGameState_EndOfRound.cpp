#include "ScoredGameState_EndOfRound.h"
#include "ScoredGameState.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include "WordGame.h"
#include "assets.gen.h"

unsigned ScoredGameState_EndOfRound::update(float dt, float stateTime)
{
    return GameStateIndex_EndOfRoundScored;
}

unsigned ScoredGameState_EndOfRound::onEvent(unsigned eventID, const EventData& data)
{
    ScoredGameState::onAudioEvent(eventID, data);
    switch (eventID)
    {
    case EventID_EnterState:
        WordGame::playAudio(wordplay_music_sayonara, AudioChannelIndex_Music, LoopRepeat);
        WordGame::playAudio(teeth_close, AudioChannelIndex_Teeth);
        break;

    case EventID_Shake:
        if (GameStateMachine::getTime() > TEETH_ANIM_LENGTH)
        {
            WordGame::playAudio(shake, AudioChannelIndex_Shake);
            return GameStateIndex_StartOfRoundScored;
        }
        break;

    default:
        break;
    }
    return GameStateIndex_EndOfRoundScored;
}
