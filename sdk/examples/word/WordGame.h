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

private:
    GameStateMachine mGameStateMachine;
};

#endif // WORDGAME_H
