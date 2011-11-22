#include <sifteo.h>
#include "WordGame.h"

using namespace Sifteo;

WordGame::WordGame(Cube cubes[]) : mGameStateMachine(cubes)
{
}

void WordGame::onEvent(int eventID)
{
    mGameStateMachine.onEvent(eventID);
}
