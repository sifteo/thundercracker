#ifndef CUBESTATE_H
#define CUBESTATE_H

#include <sifteo.h>
#include "State.h"
using namespace Sifteo;

class CubeState : public State
{
public:
    CubeState() : mCube(0) {}
    void setCube(Cube* cube);
    //virtual void onEnter();

protected:
    Cube* mCube;
};

#endif // CUBESTATE_H
