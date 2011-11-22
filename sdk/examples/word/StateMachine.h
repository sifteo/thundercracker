#ifndef STATEMACHINE_H
#define STATEMACHINE_H

class State;

class StateMachine
{
public:
    StateMachine(int startState);

    void update(float dt);
    void onEvent(int eventID);

protected:
    State* getCurrentState() const { return getState(mStateIndex); }
    virtual unsigned getNumStates() const { return 0; }
    virtual State* getState(unsigned index) const { return 0; }

private:
    void setState(unsigned newStateIndex, State* oldState);

    int mStateIndex;
    float mStateTime;
    bool mUnhandledOnEnter;
};

#endif // STATEMACHINE_H
