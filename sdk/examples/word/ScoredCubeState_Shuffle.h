#ifndef SCOREDCUBESTATE_SHUFFLE_H
#define SCOREDCUBESTATE_SHUFFLE_H

#include "ScoredCubeState.h"


class ScoredCubeState_Shuffle : public ScoredCubeState
{
public:
    virtual unsigned onEvent(unsigned eventID, const EventData& data);
    virtual unsigned update(float dt, float stateTime);

private:
    void paint();
};

#endif // SCOREDCUBESTATE_SHUFFLE_H
