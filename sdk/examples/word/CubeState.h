#ifndef CUBESTATE_H
#define CUBESTATE_H

#include "State.h"

class CubeStateMachine;

class CubeState : public State
{
public:
    CubeState() : mStateMachine(0) { }
    void setStateMachine(CubeStateMachine& csm);
    CubeStateMachine& getStateMachine();

private:
    CubeStateMachine* mStateMachine;
};

#endif // CUBESTATE_H
