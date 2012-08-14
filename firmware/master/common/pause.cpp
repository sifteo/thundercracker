#include "pause.h"
#include "bits.h"
#include "macros.h"
#include "tasks.h"
#include "homebutton.h"
#include "led.h"
#include "ledsequencer.h"
#include "cube.h"
#include "event.h"

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
            onButtonChange();
            break;

        case ButtonHold:
            monitorButtonHold();
            break;

        case LowBattery:
            lowBattery();
            break;

        }
    }
}

void Pause::onButtonChange()
{
    /*
     * Never display the pause menu within the launcher.
     * Instead, send it a GameMenu event to indicate the button was pressed.
     * Continue triggering the pause task until button is either released
     * or we reach the shutdown threshold.
     */
    if (SvmLoader::getRunLevel() == SvmLoader::RUNLEVEL_LAUNCHER) {

        if (HomeButton::isPressed()) {
            // start monitoring button hold for shutdown
            Event::setBasePending(Event::PID_BASE_GAME_MENU);
            taskWork.atomicMark(ButtonHold);
            Tasks::trigger(Tasks::Pause);
        }
        return;
    }

    /*
     * Game is running, and button was pressed - execute the pause menu.
     */
    if (HomeButton::isPressed()) {
        const uint32_t excludedTasks =
            Intrinsic::LZ(Tasks::AudioPull) |
            Intrinsic::LZ(Tasks::Pause);

        UICoordinator uic(excludedTasks);
        runPauseMenu(uic);
    }
}

void Pause::monitorButtonHold()
{
    /*
     * Shutdown on button press threshold.
     *
     * We only need to monitor this asynchronously because we want to allow
     * the launcher to keep running, and respond to our initial button press
     * in case it needs to unpair a cube (or do anything else) before the
     * shutdown timeout is reached.
     *
     * Pause contexts that the use the firmware UI loop can monitor
     * button hold in that loop.
     */
    if (HomeButton::pressDuration() > SysTime::msTicks(1000)) {

        const uint32_t excludedTasks =
            Intrinsic::LZ(Tasks::AudioPull) |
            Intrinsic::LZ(Tasks::Pause);

        SvmClock::pause();
        UICoordinator uic(excludedTasks);
        UIShutdown uiShutdown(uic);
        uiShutdown.init();
        return uiShutdown.mainLoop();
    }

    taskWork.atomicMark(ButtonHold);
    Tasks::trigger(Tasks::Pause);
}

void Pause::runPauseMenu(UICoordinator &uic)
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

    UIPause uiPause(uic);
    if (uic.isAttached())
        uiPause.init();

    LED::set(LEDPatterns::paused, true);

    do {

        uic.stippleCubes(uic.connectCubes());

        if (uic.pollForAttach())
            uiPause.init();

        uiPause.animate();
        uic.paint();

        // Long press- shut down
        if (HomeButton::pressDuration() > SysTime::msTicks(1000)) {
            UIShutdown uiShutdown(uic);
            uiShutdown.init();
            return uiShutdown.mainLoop();
        }

    } while (!(HomeButton::isReleased() && uiPause.isDone()));

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
        runPauseMenu(uic);
    }
}
