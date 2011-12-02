#ifndef SCOREDGAMESTATE_H
#define SCOREDGAMESTATE_H

#include "State.h"

const float ANAGRAM_COOLDOWN = 2.0f; // TODO reduce when tilt bug is gone

class ScoredGameState : public State
{
public:
    ScoredGameState() : mAnagramCooldown(ANAGRAM_COOLDOWN) {}
    virtual unsigned update(float dt, float stateTime);
    virtual unsigned onEvent(unsigned eventID, const EventData& data);

private:
    float mAnagramCooldown;
};

#endif // SCOREDGAMESTATE_H
