#ifndef CUBESTATE_H
#define CUBESTATE_H

#include <sifteo.h>
#include "State.h"
#include "TileTransparencyLookup.h"

enum CubeStateIndex
{
    CubeStateIndex_Title,
    CubeStateIndex_TitleExit,
    CubeStateIndex_NotWordScored,
    CubeStateIndex_NewWordScored,
    CubeStateIndex_OldWordScored,
    CubeStateIndex_StartOfRoundScored,
    CubeStateIndex_EndOfRoundScored,
    CubeStateIndex_ShuffleScored,

    CubeStateIndex_NumStates
};

class CubeStateMachine;
const float TEETH_ANIM_LENGTH = 1.7f;

class CubeState : public State
{
public:
    CubeState() :
        mStateMachine(0) { }

    void setStateMachine(CubeStateMachine& csm);
    CubeStateMachine& getStateMachine();

protected:
    virtual unsigned update(float dt, float stateTime);

private:
    CubeStateMachine* mStateMachine;
};

#endif // CUBESTATE_H
