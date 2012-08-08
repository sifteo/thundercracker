#include "pause.h"
#include "bits.h"
#include "macros.h"
#include "tasks.h"
#include "homebutton.h"
#include "led.h"
#include "ledsequencer.h"
#include "cube.h"

#include "svmloader.h"
#include "svmclock.h"

#include "ui_pause.h"
#include "ui_cuberange.h"
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
            // only want to execute on press, not release
            if (HomeButton::isPressed()) {

                const uint32_t excludedTasks =
                    Intrinsic::LZ(Tasks::AudioPull) |
                    Intrinsic::LZ(Tasks::Pause);
                UICoordinator uic(excludedTasks);
                buttonPress(uic);
            }
            break;

        case LowBattery:
            lowBattery();
            break;

        }
    }
}

void Pause::buttonPress(UICoordinator &uic)
{
    /*
     * Long running button press handler.
     * We may be running here as a result of one of several actions:
     * - homebutton press during gameplay
     * - cubes were reconnected while the cubeRange pause menu was being displayed
     *
     * The passed in UICoordinator should be configured based on system state
     * when we initially entered any pause state.
     */

    if (!SvmClock::isPaused())
        SvmClock::pause();

    UIShutdown uiShutdown(uic);

    // Immediate shutdown if the button is pressed from the launcher
    if (SvmLoader::getRunLevel() == SvmLoader::RUNLEVEL_LAUNCHER)
        return uiShutdown.mainLoop();

    UIPause uiPause(uic);
    LED::set(LEDPatterns::paused, true);
    HomeButtonPressDetector press;

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
    /*
     * A disconnection event has brought the number of cubes connected
     * to the system below the game's specified cube range.
     *
     * Pause until the user either connects enough cubes,
     * or quits back to the launcher.
     */

    const uint32_t excludedTasks =
        Intrinsic::LZ(Tasks::AudioPull)  |
        Intrinsic::LZ(Tasks::Pause);

    SvmClock::pause();
    LED::set(LEDPatterns::paused, true);

    UICoordinator uic(excludedTasks);
    UICubeRange uiCubeRange(uic);

    do {

        uic.stippleCubes(uic.connectCubes());

        if (uic.pollForAttach())
            uiCubeRange.init();

        uiCubeRange.animate();
        uic.paint();

    } while (CubeSlots::belowCubeRange() && !uiCubeRange.quitWasSelected());

    /*
     * 2 options: enough cubes were reconnected, or user selected quit.
     *
     * If cubes were reconnected, we transition to the pause menu.
     * If quit, then quit :)
     */

    if (uiCubeRange.quitWasSelected()) {
        uic.restoreCubes(uic.uiConnected);
        LED::set(LEDPatterns::idle);
        SvmClock::resume();
        SvmLoader::exit();
    } else {
        buttonPress(uic);
    }
}
