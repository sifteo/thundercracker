#ifndef STATEMACHINE_H
#define STATEMACHINE_H

class State;
union EventData;

class StateMachine
{
public:
    StateMachine(unsigned startState);

    virtual void update(float dt);
    virtual unsigned onEvent(unsigned eventID, const EventData& data) = 0;
    unsigned getCurrentStateIndex() const { return mStateIndex; }
    float getTime() const { return mStateTime; }

protected:
    virtual unsigned getNumStates() const = 0;
    virtual void setState(unsigned newStateIndex, unsigned oldStateIndex);
    float mStateTime;

private:

    unsigned mStateIndex;
    bool mUnhandledOnEnter;
};

#endif // STATEMACHINE_H
