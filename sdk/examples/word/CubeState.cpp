#include "CubeState.h"

void CubeState::setCube(Cube* cube)
{
    ASSERT(cube > 0);
    mCube = cube;
}
