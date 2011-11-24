#ifndef CUBESTATEMACHINE_H
#define CUBESTATEMACHINE_H

#include <sifteo.h>
#include "StateMachine.h"
#include "ScoredCubeState.h"

using namespace Sifteo;

const unsigned MAX_LETTERS_PER_CUBE = 1;

class CubeStateMachine : public StateMachine
{
public:
    CubeStateMachine() : StateMachine(0), mNumLetters(1), mCube(0) {}

    void setCube(Cube& cube);
    Cube& getCube();

    virtual unsigned getNumStates() const { return 1; }
    virtual State& getState(unsigned index) { ASSERT(index == 0); return mScoredState; }

    virtual void onEvent(unsigned eventID, const EventData& data);

    const char* getLetters();

private:
    // shared state data
    char mLetters[MAX_LETTERS_PER_CUBE + 1];
    unsigned mNumLetters;

    Cube* mCube;
    ScoredCubeState mScoredState;
};

#endif // CUBESTATEMACHINE_H
