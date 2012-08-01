/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Hardware-independent Home Button handling.
 *
 * The home button needs to trigger the "pause" menu, or to shut down the
 * system if it's held down long enough.
 */

#include "macros.h"
#include "homebutton.h"
#include "led.h"
#include "systime.h"
#include "tasks.h"
#include "vram.h"
#include "cube.h"
#include "ui_coordinator.h"
#include "ui_pause.h"
#include "svmloader.h"
#include "svmclock.h"
#include "shutdown.h"


void HomeButtonPressDetector::update()
{
    switch (state) {

        case S_UNKNOWN:
            if (!HomeButton::isPressed())
                state = S_IDLE;
            break;

        case S_IDLE:
        case S_RELEASED:
            if (HomeButton::isPressed()) {
                state = S_PRESSED;
                pressTimestamp = SysTime::ticks();
            }
            break;

        case S_PRESSED:
            if (!HomeButton::isPressed()) {
                state = S_RELEASED;
            }
            break;
    }
}


void HomeButton::task()
{
    const uint32_t excludedTasks =
        Intrinsic::LZ(Tasks::AudioPull)  |
        Intrinsic::LZ(Tasks::HomeButton);

    if (!isPressed())
        return;

    // Immediate shutdown if the button is pressed from the launcher
    if (SvmLoader::getRunLevel() == SvmLoader::RUNLEVEL_LAUNCHER) {
        ShutdownManager s(excludedTasks);
        return s.shutdown();
    }

    SvmClock::pause();
    LED::set(LEDPatterns::paused, true);

    UICoordinator uic(excludedTasks);
    UIPause ui(uic);
    HomeButtonPressDetector press;

    do {

        uic.stippleCubes(uic.connectCubes());

        if (uic.pollForAttach())
            ui.init();

        ui.animate();
        uic.paint();
        press.update();

        if (press.pressDuration() > SysTime::msTicks(1000)) {
            // Long press- shut down
            ShutdownManager s(excludedTasks);
            return s.shutdown();
        }

    } while (!press.isReleased() && !ui.isDone());

    uic.restoreCubes(uic.uiConnected);
    LED::set(LEDPatterns::idle);
    Tasks::cancel(Tasks::HomeButton);
    SvmClock::resume();
    ui.takeAction();
}
