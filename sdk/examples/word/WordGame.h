#ifndef WORDGAME_H
#define WORDGAME_H

#include <sifteo.h>
#include "GameStateMachine.h"
using namespace Sifteo;

union EventData;

class WordGame
{
public:
    WordGame(Cube cubes[]);
    void update(float dt);
    void onEvent(unsigned eventID, const EventData& data);

    static unsigned rand(unsigned max);

private:
    GameStateMachine mGameStateMachine;
};

#endif // WORDGAME_H
