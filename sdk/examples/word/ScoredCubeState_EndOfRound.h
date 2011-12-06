#ifndef SCOREDCUBESTATE_ENDOFROUND_H
#define SCOREDCUBESTATE_ENDOFROUND_H

#include "ScoredCubeState.h"


class ScoredCubeState_EndOfRound : public ScoredCubeState
{
public:
    virtual unsigned onEvent(unsigned eventID, const EventData& data);
    virtual unsigned update(float dt, float stateTime);

private:
    void paint();
};

#endif // SCOREDCUBESTATE_ENDOFROUND_H
