#include "StoryGameState_CityProgression.h"
#include "ScoredGameState.h"
#include "EventID.h"
#include "EventData.h"
#include "Dictionary.h"
#include "GameStateMachine.h"
#include "WordGame.h"
#include "assets.gen.h"

unsigned StoryGameState_CityProgression::update(float dt, float stateTime)
{

    return GameStateIndex_StoryCityProgression;
}

unsigned StoryGameState_CityProgression::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_Shake:
        WordGame::playAudio(shake, AudioChannelIndex_Shake);
        return GameStateIndex_ShuffleScored;
    }
    return GameStateIndex_StoryCityProgression;
}
