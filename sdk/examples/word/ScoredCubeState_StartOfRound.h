#ifndef SCOREDCUBESTATE_STARTOFROUND_H
#define SCOREDCUBESTATE_STARTOFROUND_H

#include "ScoredCubeState.h"

class ScoredCubeState_StartOfRound : public ScoredCubeState
{
public:
    ScoredCubeState_StartOfRound();

    virtual unsigned onEvent(unsigned eventID, const EventData& data);
    virtual unsigned update(float dt, float stateTime);

private:
    void paint();

};
#endif // SCOREDCUBESTATE_STARTOFROUND_H
