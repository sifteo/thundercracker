#ifndef PAUSE_H
#define PAUSE_H

#include <stdint.h>
#include "bits.h"
#include "ui_coordinator.h"

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
    static void buttonPress(UICoordinator &uic);
    static void lowBattery();
};

#endif // PAUSE_H
