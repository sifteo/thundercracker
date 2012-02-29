#ifndef STORYGAMESTATE_CITYPROGRESSION_H
#define STORYGAMESTATE_CITYPROGRESSION_H

#include "State.h"

class StoryGameState_CityProgression : public State
{
public:
    virtual unsigned update(float dt, float stateTime);
    virtual unsigned onEvent(unsigned eventID, const EventData& data);

};

#endif // STORYGAMESTATE_CITYPROGRESSION_H
