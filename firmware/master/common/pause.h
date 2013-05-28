/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

#ifndef PAUSE_H
#define PAUSE_H

#include <stdint.h>
#include "bits.h"
#include "ui_coordinator.h"
#include "ui_pause.h"
#include "ui_cuberange.h"
#include "ui_lowbatt.h"
#include "ui_bluetoothpairing.h"
#include "homebutton.h"

class Pause {
public:
    enum WorkItem {
        ButtonPress,
        ButtonHold,
        BluetoothPairing,
        LowBattery,
        NUM_WORK_ITEMS,         // Must be last
    };

    static BitVector<NUM_WORK_ITEMS> taskWork;

    enum Mode {
        ModePause,
        ModeCubeRange,
        ModeLowBattery,
        ModeBluetoothPairing
    };

    static void task();
    static void mainLoop(Mode mode);
    static void beginBluetoothPairing();

private:
    static ALWAYS_INLINE void onButtonChange();
    static ALWAYS_INLINE void monitorButtonHold();

    static uint32_t prepForPause();
    static bool pauseModeHandler(UICoordinator &uic, UIPause &uip, Mode &mode);
    static bool cubeRangeModeHandler(UICoordinator &uic, UICubeRange &uicr, Mode &mode);
    static bool lowBatteryModeHandler(UICoordinator &uic, UILowBatt &uilb, Mode &mode);
    static bool bluetoothPairingModeHandler(UICoordinator &uic, UIBluetoothPairing &uibp, Mode &mode);
    static void cleanup(UICoordinator &uic);
//    static void updateNextMode(Mode &mode);
};

#endif // PAUSE_H
