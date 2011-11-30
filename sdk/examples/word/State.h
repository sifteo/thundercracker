#ifndef STATE_H
#define STATE_H

union EventData;

class State
{
public:
    State();
    virtual unsigned update(float dt, float stateTime) = 0;
    virtual unsigned onEvent(unsigned eventID, const EventData& data) = 0;
};

#endif // STATE_H
