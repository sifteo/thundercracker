#ifndef SCOREDCUBESTATE_OLDWORD_H
#define SCOREDCUBESTATE_OLDWORD_H

#include "ScoredCubeState.h"

class ScoredCubeState_OldWord : public ScoredCubeState
{
public:
    ScoredCubeState_OldWord();

    virtual unsigned onEvent(unsigned eventID, const EventData& data);
    virtual unsigned update(float dt, float stateTime);

private:
    void paint();
};

#endif // SCOREDCUBESTATE_OLDWORD_H
