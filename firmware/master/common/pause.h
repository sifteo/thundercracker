#ifndef PAUSE_H
#define PAUSE_H

#include <stdint.h>
#include "bits.h"

class Pause {
public:
    enum WorkItems {
        ButtonPress,
        LowBattery,
        CubeRange,

        NUM_WORK_ITEMS,         // Must be last
    };

    static BitVector<NUM_WORK_ITEMS> taskWork;

    static void task();

private:

    static void buttonPress();
    static void lowBattery();
    static void cubeRange();

};

#endif // PAUSE_H
