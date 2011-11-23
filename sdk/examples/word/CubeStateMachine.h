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
    void setCube(Cube& cube);

    virtual unsigned getNumStates() const { return 1; }
    virtual State& getState(unsigned index) { ASSERT(index == 0); return mScoredState; }


private:
    Cube* mCube;
    ScoredCubeState mScoredState;
};

#endif // CUBESTATEMACHINE_H
