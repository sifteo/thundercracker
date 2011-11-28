#ifndef SCOREDCUBESTATE_H
#define SCOREDCUBESTATE_H

#include "CubeState.h"

enum ScoredCubeSubstate
{
    ScoredCubeSubstate_NotWord,
    ScoredCubeSubstate_NewWord,
    ScoredCubeSubstate_OldWord,

    ScoredCubeSubstate_NumStates
};

class ScoredCubeState : public CubeState
{
public:
    ScoredCubeState();

    virtual unsigned onEvent(unsigned eventID, const EventData& data);

};

#endif // SCOREDCUBESTATE_H
