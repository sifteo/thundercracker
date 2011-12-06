#include "TitleGameState.h"
#include "ScoredGameState.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include <string.h>
#include "WordGame.h"
#include "audio.gen.h"

unsigned TitleGameState::update(float dt, float stateTime)
{
    return GameStateIndex_Title;
}

unsigned TitleGameState::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_EnterState:
        GameStateMachine::sOnEvent(EventID_Title, EventData());
        WordGame::playAudio(welcome, AudioChannelIndex_Time);
        WordGame::playAudio(wordplay_music_sohcahtoa, AudioChannelIndex_Music, LoopRepeat);
        break;

    case EventID_Input:
        return GameStateIndex_PlayScored;

    default:
        break;
    }
    return GameStateIndex_Title;
}
