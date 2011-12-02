#ifndef SCOREDCUBESTATE_H
#define SCOREDCUBESTATE_H

#include "CubeState.h"

enum ScoredCubeStateIndex
{
    ScoredCubeStateIndex_NotWord,
    ScoredCubeStateIndex_NewWord,
    ScoredCubeStateIndex_OldWord,
    ScoredCubeStateIndex_EndOfRound,

    ScoredCubeStateIndex_NumStates
};

class ScoredCubeState : public CubeState
{
public:
    ScoredCubeState();

    virtual unsigned onEvent(unsigned eventID, const EventData& data);

};

#endif // SCOREDCUBESTATE_H
