#ifndef SCOREDGAMESTATE_H
#define SCOREDGAMESTATE_H

#include "State.h"
#include "config.h"
#include "Constants.h"


class ScoredGameState : public State
{
public:    
    ScoredGameState();
    virtual unsigned update(float dt, float stateTime);
    virtual unsigned onEvent(unsigned eventID, const EventData& data);



private:

};

#endif // SCOREDGAMESTATE_H
