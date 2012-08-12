#ifndef PAUSE_H
#define PAUSE_H

#include <stdint.h>
#include "bits.h"
#include "ui_coordinator.h"
#include "homebutton.h"

class Pause {
public:
    enum WorkItems {
        ButtonPress,
        LowBattery,

        NUM_WORK_ITEMS,         // Must be last
    };

    static BitVector<NUM_WORK_ITEMS> taskWork;

    static void task();
    static void cubeRange();

private:
    static void runPauseMenu(UICoordinator &uic);
    static ALWAYS_INLINE void onButtonChange();
    static ALWAYS_INLINE void lowBattery();

    static HomeButtonPressDetector press;
};

#endif // PAUSE_H
