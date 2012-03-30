#include "TitleGameState.h"
#include "ScoredGameState.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include "WordGame.h"
#include "assets.gen.h"

unsigned TitleGameState::update(float dt, float stateTime)
{
    return GameStateIndex_Title;
}

unsigned TitleGameState::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_EnterState:
        WordGame::playAudio(flap_laugh_fireball, AudioChannelIndex_Time);
        WordGame::playAudio(wordplay_music_sayonara, AudioChannelIndex_Music, LoopRepeat);
        break;

    case EventID_Start:
        return GameStateIndex_StartOfRoundScored;

    default:
        break;
    }
    return GameStateIndex_Title;
}
