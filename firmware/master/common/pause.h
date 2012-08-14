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
        ButtonHold,
        LowBattery,

        NUM_WORK_ITEMS,         // Must be last
    };

    static BitVector<NUM_WORK_ITEMS> taskWork;

    static void task();
    static void cubeRange();

private:
    static void runPauseMenu(UICoordinator &uic);
    static ALWAYS_INLINE void onButtonChange();
    static ALWAYS_INLINE void monitorButtonHold();
    static ALWAYS_INLINE void lowBattery();
};

#endif // PAUSE_H
