#ifndef STATE_H
#define STATE_H

class State
{
public:
    State();
    virtual void OnEnter() {}
    virtual void OnExit() {}
    virtual int Update(float dt, float stateTime) { return 0; }
};

#endif // STATE_H
