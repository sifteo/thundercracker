#ifndef TITLECUBESTATE_H
#define TITLECUBESTATE_H

#include "CubeState.h"

class TitleCubeState : public CubeState
{
public:
    TitleCubeState() : mShakeDelay(0.f), mPanning(0.f) {}
    virtual unsigned onEvent(unsigned eventID, const EventData& data);
    virtual unsigned update(float dt, float stateTime);

private:
    void paint();
    float mShakeDelay;
    float mPanning;
};

#endif // TITLECUBESTATE_H
