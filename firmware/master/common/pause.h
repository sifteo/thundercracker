/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2013 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

    static uint32_t prepareToPause();
    static bool pauseModeHandler(UICoordinator &uic, UIPause &uip, Mode &mode);
    static bool cubeRangeModeHandler(UICoordinator &uic, UICubeRange &uicr, Mode &mode);
    static bool lowBatteryModeHandler(UICoordinator &uic, UILowBatt &uilb, Mode &mode);
    static bool bluetoothPairingModeHandler(UICoordinator &uic, UIBluetoothPairing &uibp, Mode &mode);
    static void cleanup(UICoordinator &uic);
//    static void updateNextMode(Mode &mode);
};

#endif // PAUSE_H
