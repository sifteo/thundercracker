#ifndef GAMESTATEMACHINE_H
#define GAMESTATEMACHINE_H

#include <sifteo.h>
#include "StateMachine.h"
#include "ScoredGameState.h"
#include "CubeStateMachine.h"
using namespace Sifteo;

// HACK workaround inability to check if a Cube is actually connected
const unsigned MAX_CUBES = 6;

class GameStateMachine : public StateMachine
{
public:
    GameStateMachine(Cube cubes[]);

    virtual void update(float dt);
    virtual void onEvent(unsigned eventID, const EventData& data);
    static void sOnEvent(unsigned eventID, const EventData& data);
    static unsigned GetNumCubes() { return MAX_CUBES; }// TODO

protected:
    virtual State& getState(unsigned index) { ASSERT(index == 0); return mScoredState; }
    virtual unsigned getNumStates() const { return 1; }


private:
    ScoredGameState mScoredState;
    CubeStateMachine mCubeStateMachines[MAX_CUBES];
    static GameStateMachine* sInstance;
};

#endif // GAMESTATEMACHINE_H
