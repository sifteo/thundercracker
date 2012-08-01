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
#include "ui_shutdown.h"
#include "svmloader.h"
#include "svmclock.h"
#include "shutdown.h"


void HomeButtonPressDetector::update()
{
    bool pressed = HomeButton::isPressed();

    if (!pressed)
        pressTimestamp = 0;
    else if (!pressTimestamp)
        pressTimestamp = SysTime::ticks();

    switch (state) {
        case S_UNKNOWN:
            if (!pressed)
                state = S_IDLE;
            break;

        case S_IDLE:
        case S_RELEASED:
            if (pressed)
                state = S_PRESSED;
            break;

        case S_PRESSED:
            if (!pressed)
                state = S_RELEASED;
            break;
    }
}

SysTime::Ticks HomeButtonPressDetector::pressDuration() const
{
    return HomeButton::isPressed() ? (SysTime::ticks() - pressTimestamp) : 0;
}


void HomeButton::task()
{
    const uint32_t excludedTasks =
        Intrinsic::LZ(Tasks::AudioPull)  |
        Intrinsic::LZ(Tasks::HomeButton);

    if (!isPressed())
        return;

    SvmClock::pause();
    LED::set(LEDPatterns::paused, true);

    UICoordinator uic(excludedTasks);
    UIPause uiPause(uic);
    UIShutdown uiShutdown(uic);
    HomeButtonPressDetector press;

    // Immediate shutdown if the button is pressed from the launcher
    if (SvmLoader::getRunLevel() == SvmLoader::RUNLEVEL_LAUNCHER)
        return uiShutdown.mainLoop();

    do {

        uic.stippleCubes(uic.connectCubes());

        if (uic.pollForAttach())
            uiPause.init();

        uiPause.animate();
        uic.paint();
        press.update();

        // Long press- shut down
        if (press.pressDuration() > SysTime::msTicks(1000)) {
            uiShutdown.init();
            return uiShutdown.mainLoop();
        }

    } while (!press.isReleased() && !uiPause.isDone());

    uic.restoreCubes(uic.uiConnected);
    LED::set(LEDPatterns::idle);
    Tasks::cancel(Tasks::HomeButton);
    SvmClock::resume();
    uiPause.takeAction();
}
