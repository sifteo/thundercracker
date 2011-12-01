#include <sifteo.h>
#include "WordGame.h"
#include <cstdlib>

using namespace Sifteo;

WordGame::WordGame(Cube cubes[]) : mGameStateMachine(cubes)
{
#ifdef _WIN32
    int64_t nanosec;
    _SYS_ticks_ns(&nanosec);
    unsigned seed = (unsigned)nanosec;
    srand(seed); // seed rand()
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
