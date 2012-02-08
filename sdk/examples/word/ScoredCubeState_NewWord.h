#ifndef SCOREDCUBESTATE_NEWWORD_H
#define SCOREDCUBESTATE_NEWWORD_H

#include "ScoredCubeState.h"
#include "TileTransparencyLookup.h"

class ScoredCubeState_NewWord : public ScoredCubeState
{
public:
    ScoredCubeState_NewWord();

    virtual unsigned onEvent(unsigned eventID, const EventData& data);
    virtual unsigned update(float dt, float stateTime);

private:
    void paint();

    ImageIndex mImageIndex;

};

#endif // SCOREDCUBESTATE_NEWWORD_H
