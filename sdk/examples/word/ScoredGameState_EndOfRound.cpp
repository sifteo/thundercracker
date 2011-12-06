#include "ScoredGameState_EndOfRound.h"
#include "ScoredGameState.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include <string.h>
#include "WordGame.h"
#include "audio.gen.h"

unsigned ScoredGameState_EndOfRound::update(float dt, float stateTime)
{
    return ScoredGameStateIndex_EndOfRound;
}

unsigned ScoredGameState_EndOfRound::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_EnterState:
        GameStateMachine::sOnEvent(EventID_EndRound, EventData());
        WordGame::playAudio(wordplay_music_sayonara, AudioChannelIndex_Music, LoopRepeat);
        break;

    case EventID_Input:
        return ScoredGameStateIndex_Play;

    default:
        break;
    }
    return ScoredGameStateIndex_EndOfRound;
}
