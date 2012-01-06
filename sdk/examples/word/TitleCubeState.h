#ifndef TITLECUBESTATE_H
#define TITLECUBESTATE_H

#include "CubeState.h"

class TitleCubeState : public CubeState
{
public:
    TitleCubeState() : mAnimDelay(0.f), mAnimStartTime(0.f), mAnimStart(false) {}
    virtual unsigned onEvent(unsigned eventID, const EventData& data);
    virtual unsigned update(float dt, float stateTime);

private:
    void paint();
    float mAnimDelay;
    float mAnimStartTime;
    bool mAnimStart;
};

#endif // TITLECUBESTATE_H
