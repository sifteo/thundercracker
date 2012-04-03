#ifndef SCOREDGAMESTATE_SHUFFLE_H
#define SCOREDGAMESTATE_SHUFFLE_H

#include "State.h"

class ScoredGameState_Shuffle : public State
{
public:
    virtual unsigned update(float dt, float stateTime);
    virtual unsigned onEvent(unsigned eventID, const EventData& data);

private:
};

#endif // SCOREDGAMESTATE_SHUFFLE_H
