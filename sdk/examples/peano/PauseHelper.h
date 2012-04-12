#pragma once

#include "Game.h"

namespace TotalsGame {

class PauseHelper {
    // coalescing all button presses together -- not totally correct,
    // but much simpler and efficient.

    float mSeconds;

public:

    PauseHelper()
    {
        mSeconds = 0;
    }

    void Update() {
        for(int i=0; i<NUM_CUBES; ++i) {
            if (Game::cubes[i].touching()) {
                mSeconds += Game::dt;
                return;
            }
        }
        mSeconds = 0;
    }

    void Reset() {
        mSeconds = 0;
    }

    bool IsTriggered()
    {
        return mSeconds > 1.0f;
    }
};



}

