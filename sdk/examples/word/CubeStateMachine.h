#ifndef CUBESTATEMACHINE_H
#define CUBESTATEMACHINE_H

#include <sifteo.h>
#include "StateMachine.h"
#include "ScoredCubeState.h"

using namespace Sifteo;

class CubeStateMachine : public StateMachine
{
public:
    CubeStateMachine() : StateMachine(0), mCube(0) {}
    void setCube(Cube* cube);

private:
    Cube* mCube;
    ScoredCubeState mScoredState;
};

#endif // CUBESTATEMACHINE_H
