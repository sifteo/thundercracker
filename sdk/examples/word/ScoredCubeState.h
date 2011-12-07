#ifndef SCOREDCUBESTATE_H
#define SCOREDCUBESTATE_H

#include "CubeState.h"

class ScoredCubeState : public CubeState
{
public:
    ScoredCubeState();

    virtual unsigned onEvent(unsigned eventID, const EventData& data);

};

#endif // SCOREDCUBESTATE_H
