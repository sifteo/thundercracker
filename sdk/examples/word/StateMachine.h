#ifndef STATEMACHINE_H
#define STATEMACHINE_H

class State;

class StateMachine
{
public:
    StateMachine(unsigned startState);

    void update(float dt);
    void onEvent(unsigned eventID);

protected:
    State* getCurrentState() { return getState(mStateIndex); }
    virtual unsigned getNumStates() const = 0;
    virtual State* getState(unsigned index) = 0;

private:
    void setState(unsigned newStateIndex, State* oldState);

    unsigned mStateIndex;
    float mStateTime;
    bool mUnhandledOnEnter;
};

#endif // STATEMACHINE_H
