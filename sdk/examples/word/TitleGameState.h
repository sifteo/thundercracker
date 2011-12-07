#ifndef TITLEGAMESTATE_H
#define TITLEGAMESTATE_H

#include "State.h"

class TitleGameState : public State
{
public:
public:
    virtual unsigned update(float dt, float stateTime);
    virtual unsigned onEvent(unsigned eventID, const EventData& data);
};

#endif // TITLEGAMESTATE_H
