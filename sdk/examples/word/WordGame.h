#ifndef WORDGAME_H
#define WORDGAME_H

#ifndef _SIFTEO_CUBE_H
class Cube;
#endif

const unsigned MAX_CUBES = 32;

#include "GameStateMachine.h"

using namespace Sifteo;

class WordGame
{
public:
    WordGame(Cube cubes[]);
    void Update(float dt) {}
    void Paint() {}

private:
    GameStateMachine mStateMachine;
    Cube* mCubes;
};

#endif // WORDGAME_H
