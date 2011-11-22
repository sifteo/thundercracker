#ifndef WORDGAME_H
#define WORDGAME_H

#ifndef _SIFTEO_CUBE_H
class Cube;
#endif

using namespace Sifteo;

class WordGame
{
public:
    WordGame(Cube cubes[]);
    void Update(float dt) {}
    void Paint() {}
};

#endif // WORDGAME_H
