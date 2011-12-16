#ifndef TITLEEXITCUBESTATE_H
#define TITLEEXITCUBESTATE_H

#include "CubeState.h"

class TitleExitCubeState : public CubeState
{
public:
    virtual unsigned onEvent(unsigned eventID, const EventData& data);
    virtual unsigned update(float dt, float stateTime);

private:
    void paint();
};

#endif // TITLEEXITCUBESTATE_H
