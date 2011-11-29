#ifndef SCOREDCUBESTATE_NOTWORD_H
#define SCOREDCUBESTATE_NOTWORD_H

#include "ScoredCubeState.h"

class ScoredCubeState_NotWord : public ScoredCubeState
{
public:
    ScoredCubeState_NotWord();

    virtual unsigned onEvent(unsigned eventID, const EventData& data);
    virtual unsigned update(float dt, float stateTime);

private:
    void paint();

};

#endif // SCOREDCUBESTATE_NOTWORD_H
