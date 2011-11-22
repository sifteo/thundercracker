#include "ScoredCubeState.h"
#include "EventID.h"

ScoredCubeState::ScoredCubeState()
{
}

unsigned ScoredCubeState::onEvent(unsigned eventID)
{
    switch (eventID)
    {
    case EventID_Paint:
        // TODO paint
        break;

    }
    return 0;
}
