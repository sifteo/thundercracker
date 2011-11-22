#include <sifteo.h>
#include "WordGame.h"

using namespace Sifteo;

WordGame::WordGame(Cube cubes[]) : mGameStateMachine(cubes)
{
}

void WordGame::update(float dt)
{
    mGameStateMachine.update(dt);
}

void WordGame::onEvent(unsigned eventID)
{
    mGameStateMachine.onEvent(eventID);
}
