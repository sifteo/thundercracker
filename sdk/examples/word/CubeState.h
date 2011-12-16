#ifndef CUBESTATE_H
#define CUBESTATE_H

#include <sifteo.h>
#include "State.h"

enum CubeStateIndex
{
    CubeStateIndex_Title,
    CubeStateIndex_NotWordScored,
    CubeStateIndex_NewWordScored,
    CubeStateIndex_OldWordScored,
    CubeStateIndex_EndOfRoundScored,

    CubeStateIndex_NumStates
};

class CubeStateMachine;


class CubeState : public State
{
public:
    CubeState() : mStateMachine(0) { }
    void setStateMachine(CubeStateMachine& csm);
    CubeStateMachine& getStateMachine();

protected:
    void paintTeeth(bool animate=false);
    void paintLetters(VidMode_BG0 &vid, const AssetImage &font);

private:
    CubeStateMachine* mStateMachine;
};

#endif // CUBESTATE_H
