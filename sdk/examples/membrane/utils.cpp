/* Copyright <c> 2011 Sifteo, Inc. All rights reserved. */

#include <stdlib.h>
#include <sifteo.h>

#include "game.h"


int randint(int min, int max)
{
#ifdef _WIN32
    return min + (rand()%(max-min+1));
#else
    static unsigned int seed = (int)System::clock();
    return min + (rand_r(&seed)%(max-min+1));
#endif
}

Ticker::Ticker()
    : accum(0) {}

Ticker::Ticker(float hz)
    : accum(0)
{
    setRate(hz);
}

void Ticker::setRate(float hz)
{
    period = 1.0f / hz;
}

int Ticker::tick(float dt)
{
    accum += dt;
    int frames = accum / period;
    accum -= frames * period;
    return frames;
}

Vec2 Vec2F::round()
{
    return Vec2(x + 0.5f, y + 0.5f);
}
