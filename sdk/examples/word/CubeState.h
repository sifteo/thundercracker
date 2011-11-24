#ifndef CUBESTATE_H
#define CUBESTATE_H

#include <sifteo.h>
#include "State.h"
using namespace Sifteo;

class CubeState : public State
{
public:
    CubeState() : mCube(0), otherCube(Cube()) { }
    void setCube(Cube& cube);

protected:
    Cube* mCube;
    const Cube& otherCube;
};

#endif // CUBESTATE_H
