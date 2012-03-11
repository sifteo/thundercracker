#ifndef STATEMACHINE_H
#define STATEMACHINE_H

class State;
union EventData;

class StateMachine
{
public:
    StateMachine(unsigned startState);

    virtual void update(float dt);
    virtual unsigned onEvent(unsigned eventID, const EventData& data);
    unsigned getCurrentStateIndex() const { return mStateIndex; }
    float getTime() const { return mStateTime; }

protected:
    State& getCurrentState() { return getState(mStateIndex); }
    virtual unsigned getNumStates() const = 0;
    virtual State& getState(unsigned index) = 0;
    virtual void setState(unsigned newStateIndex, State& oldState);
    float mStateTime;

private:

    unsigned mStateIndex;
    bool mUnhandledOnEnter;
};

#endif // STATEMACHINE_H
