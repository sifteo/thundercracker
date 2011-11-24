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
    Cube& getCube();
    virtual unsigned getNumStates() const { return 1; }
    virtual State& getState(unsigned index) { ASSERT(index == 0); return mScoredState; }

    const char* getSubString() { return "hi"; }

private:
    // shared state data
    const char* mString;
    unsigned mSubStringStart;
    unsigned mSubStringLength;

    Cube* mCube;
    ScoredCubeState mScoredState;
};

#endif // CUBESTATEMACHINE_H
