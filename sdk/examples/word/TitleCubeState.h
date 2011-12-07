#ifndef TITLECUBESTATE_H
#define TITLECUBESTATE_H

#include "CubeState.h"

class TitleCubeState : public CubeState
{
public:
    virtual unsigned onEvent(unsigned eventID, const EventData& data);
    virtual unsigned update(float dt, float stateTime);

private:
    void paint();
};

#endif // TITLECUBESTATE_H
