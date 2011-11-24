#ifndef SCOREDCUBESTATE_H
#define SCOREDCUBESTATE_H

#include "CubeState.h"

class ScoredCubeState : public CubeState
{
public:
    ScoredCubeState();

    virtual unsigned onEvent(unsigned eventID, const EventData& data);
    virtual unsigned update(float dt, float stateTime) { return 0; }

};

#endif // SCOREDCUBESTATE_H
