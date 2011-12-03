#ifndef SCOREDGAMESTATE_H
#define SCOREDGAMESTATE_H

#include "State.h"


const float ANAGRAM_COOLDOWN = 2.0f; // TODO reduce when tilt bug is gone
const float ROUND_TIME = 12.0f; // TODO reduce when tilt bug is gone
const float ROUND_BONUS_TIME = 2.0f; // TODO reduce when tilt bug is gone

class ScoredGameState : public State
{
public:    
    virtual unsigned update(float dt, float stateTime);
    virtual unsigned onEvent(unsigned eventID, const EventData& data);

};

#endif // SCOREDGAMESTATE_H
