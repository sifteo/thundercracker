#ifndef SCOREDGAMESTATE_ENDOFROUND_H
#define SCOREDGAMESTATE_ENDOFROUND_H

#include "State.h"

class ScoredGameState_EndOfRound : public State
{
public:
    virtual unsigned update(float dt, float stateTime);
    virtual unsigned onEvent(unsigned eventID, const EventData& data);

};

#endif // SCOREDGAMESTATE_ENDOFROUND_H
