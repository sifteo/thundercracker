#include "pause.h"
#include "bits.h"
#include "macros.h"
#include "tasks.h"
#include "homebutton.h"
#include "led.h"
#include "ledsequencer.h"

#include "svmloader.h"
#include "svmclock.h"

#include "ui_pause.h"
#include "ui_shutdown.h"
#include "ui_coordinator.h"

BitVector<Pause::NUM_WORK_ITEMS> Pause::taskWork;

void Pause::task()
{
    /*
     * The system is paused for
     */

    BitVector<NUM_WORK_ITEMS> pending = taskWork;
    unsigned index;

    while (pending.clearFirst(index)) {
        taskWork.atomicClear(index);
        switch (index) {

        case ButtonPress:
            buttonPress();
            break;

        case LowBattery:
            lowBattery();
            break;

        case CubeRange:
            cubeRange();
            break;

        }
    }
}

void Pause::buttonPress()
{
    /*
     * Long running button press handler.
     * Execute the pause menu (UIPause).
     */

    const uint32_t excludedTasks =
        Intrinsic::LZ(Tasks::AudioPull)  |
        Intrinsic::LZ(Tasks::Pause);

    if (!HomeButton::isPressed())
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
    Tasks::cancel(Tasks::Pause);
    SvmClock::resume();
    uiPause.takeAction();
}

void Pause::lowBattery()
{

}

void Pause::cubeRange()
{

}
