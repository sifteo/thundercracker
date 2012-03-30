#ifndef SCOREDGAMESTATE_H
#define SCOREDGAMESTATE_H

#include <sifteo.h>
#include "State.h"
#include "config.h"
#include "Constants.h"

using namespace Sifteo;

class ScoredGameState : public State
{
public:    
    ScoredGameState();
    virtual unsigned update(float dt, float stateTime);
    virtual unsigned onEvent(unsigned eventID, const EventData& data);

    static void createNewAnagram();

    static void onAudioEvent(unsigned eventID, const EventData& data);

private:

    Cube::ID mHintCubeIDOnUpdate;
};

#endif // SCOREDGAMESTATE_H
