#ifndef SCOREDGAMESTATE_H
#define SCOREDGAMESTATE_H

#include "State.h"

class ScoredGameState : public State
{
public:
    virtual void OnEnter() {}
    virtual void OnExit() {}
    virtual int Update(float dt, float stateTime) { return 0; }
    virtual int OnEvent(int eventID) { return 0; }
};

#endif // SCOREDGAMESTATE_H
