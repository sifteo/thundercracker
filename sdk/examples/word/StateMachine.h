#ifndef STATEMACHINE_H
#define STATEMACHINE_H

class State;

class StateMachine
{
public:
    StateMachine(int startState);

    void Update(float dt);
    void OnEvent(int eventID);

protected:
    virtual State* GetState(int index) const { return 0; }
    virtual int GetNumStates() const { return 0; }

private:
    void SetState(int newStateIndex, State* oldState);

    int mStateIndex;
    float mStateTime;
};

#endif // STATEMACHINE_H
