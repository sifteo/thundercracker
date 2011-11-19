#ifndef STATEMACHINE_H
#define STATEMACHINE_H

const int MAX_STATES = 4;

class State;

class StateMachine
{
public:
    StateMachine(int startState);

    void Update(float dt);

    void AddState(State* s);

private:
    State* mStates[MAX_STATES];
    int mNumStates;
    int mStateIndex;
    float mStateTime;
};

#endif // STATEMACHINE_H
