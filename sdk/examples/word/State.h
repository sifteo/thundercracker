#ifndef STATE_H
#define STATE_H

class State
{
public:
    State();
    virtual void onEnter() {}
    virtual void onExit() {}
    virtual unsigned update(float dt, float stateTime) { return 0; }
    virtual unsigned onEvent(unsigned eventID) { return 0; }
};

#endif // STATE_H
