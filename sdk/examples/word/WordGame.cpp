#include <sifteo.h>
#include "WordGame.h"
#include <cstdlib>

using namespace Sifteo;

WordGame::WordGame(Cube cubes[]) : mGameStateMachine(cubes)
{
#ifdef _WIN32
    srand((unsigned)System::clock()); // seed rand()
#endif
}

void WordGame::update(float dt)
{
    mGameStateMachine.update(dt);
}

void WordGame::onEvent(unsigned eventID, const EventData& data)
{
    mGameStateMachine.onEvent(eventID, data);
}

unsigned WordGame::rand(unsigned max)
{
#ifdef _WIN32
 return std::rand() % max;
#else
 static unsigned int seed = (int)System::clock();
 return rand_r(&seed) % max;
#endif
}
