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

#include "ui_shutdown.h"
#include "ui_lowbatt.h"
#include "batterylevel.h"

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
            mainLoop(ModeLowBattery);
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
    if (HomeButton::isPressed())
        mainLoop(ModePause);
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

    if (HomeButton::isPressed()) {
        // continue monitoring
        taskWork.atomicMark(ButtonHold);
        Tasks::trigger(Tasks::Pause);
    }
}

void Pause::mainLoop(Mode mode)
{
    /*
     * Run the pause menu in the given mode.
     *
     * The pause menu can transition between modes - each
     * mode's handler can indicate whether we should exit from
     * the entire pause context, or whether the mode has changed.
     *
     * Upon mode change, we just need to be sure to init the UI element
     * for that mode.
     *
     * Mode handlers are responsible for detecting their completion
     * conditions, animating the UI, and ensuring that the UI is
     * being shown on a connected cube.
     */

    if (!SvmClock::isPaused())
        SvmClock::pause();

    const uint32_t excludedTasks =
        Intrinsic::LZ(Tasks::AudioPull)  |
        Intrinsic::LZ(Tasks::Pause);
    UICoordinator uic(excludedTasks);

    // all possible UI elements
    UIPause uiPause(uic);
    UICubeRange uiCubeRange(uic);
    UILowBatt uiLowBatt(uic);

    LED::set(LEDPatterns::paused, true);

    bool finished = false;
    Mode lastMode = static_cast<Mode>(0xff);  // garbage value forces an init

    /*
     * Run our loop, detecting mode changes and pumping the
     * appropriate UI element.
     */

    while (!finished) {

        bool modeChanged = lastMode != mode;
        lastMode = mode;
        uic.stippleCubes(uic.connectCubes());

        switch (mode) {

        case ModePause:
            if (modeChanged && uic.isAttached())
                uiPause.init();
            finished = pauseModeHandler(uic, uiPause, mode);
            break;

        case ModeCubeRange:
            if (modeChanged && uic.isAttached())
                uiCubeRange.init();
            finished = cubeRangeModeHandler(uic, uiCubeRange, mode);
            break;

        case ModeLowBattery:
            finished = lowBatteryModeHandler(uic, uiLowBatt, mode, modeChanged);
            break;
        }

        // Long press - always allow shut down
        if (HomeButton::pressDuration() > SysTime::msTicks(1000)) {
            UIShutdown uiShutdown(uic);
            uiShutdown.init();
            return uiShutdown.mainLoop();
        }
    }
}

bool Pause::pauseModeHandler(UICoordinator &uic, UIPause &uip, Mode &mode)
{
    if (uic.pollForAttach())
        uip.init();

    uip.animate();
    uic.paint();

    // has menu finished, and button is not still potentially being held for shutdown?
    if (uip.isDone() && HomeButton::isReleased()) {
        cleanup(uic);
        uip.takeAction();
        return true;
    }

    // Did we transition back to having too few cubes?
    if (CubeSlots::belowCubeRange())
        mode = ModeCubeRange;

    return false;
}

bool Pause::cubeRangeModeHandler(UICoordinator &uic, UICubeRange &uicr, Mode &mode)
{
    if (uic.pollForAttach())
        uicr.init();

    uicr.animate();
    uic.paint();

    // has menu finished, and button is not still potentially being held for shutdown?
    if (uicr.quitWasSelected() && HomeButton::isReleased()) {
        cleanup(uic);
        SvmLoader::exit();
        return true;
    }

    if (!CubeSlots::belowCubeRange()) {
        /*
         * CubeRange is now fulfilled.
         *
         * If we're in the launcher, there's no reason to pause, so just resume it.
         * Otherwise, transition to pause mode to give the user a chance
         * to gather their thoughts before resuming their game.
         */
        if (SvmLoader::getRunLevel() == SvmLoader::RUNLEVEL_LAUNCHER) {
            cleanup(uic);
            return true;
        }

        // No need to Pause after if we display a battery warning.
        if (BatteryLevel::needWarning()) {
            mode = ModeLowBattery;
        } else {
            mode = ModePause;
        }

    }

    return false;
}

bool Pause::lowBatteryModeHandler(UICoordinator &uic, UILowBatt &uilb, Mode &mode, bool modeChanged)
{
    static uint8_t cubeSelected = BatteryLevel::BASE + 1;
    uint8_t cid = BatteryLevel::getLowBatDevice();

    // Attach to the right cube if it's not the case.
    if (uic.pollForAttach(cid) || modeChanged) {
        uilb.init(cid);
        cubeSelected = cid;
    }

    uilb.animate();
    uic.paint();

    // has menu finished ?
    if (uilb.isDone()) {
        BatteryLevel::setWarningDone(cubeSelected);
        cleanup(uic);

        if (uilb.quitWasSelected()) {
            SvmLoader::exit();
        }
        return true;
    }

    // Did we transition back to having too few cubes?
    if (CubeSlots::belowCubeRange()) {
        mode = ModeCubeRange;
        return false;
    }

    // Pause if required, except if in launcher.
    if (HomeButton::isPressed()) {
        BatteryLevel::setWarningDone(cubeSelected);
        if (SvmLoader::getRunLevel() == SvmLoader::RUNLEVEL_LAUNCHER) {
            cleanup(uic);
            return true;
        }
        mode = ModePause;
        return false;
    }

    return false;
}

void Pause::cleanup(UICoordinator &uic)
{
    /*
     * Helper for common clean up tasks when transitioning
     * out of the pause menu altogether.
     */

    uic.restoreCubes(uic.uiConnected);
    LED::set(LEDPatterns::idle);
    Tasks::cancel(Tasks::Pause);
    if (SvmClock::isPaused())
        SvmClock::resume();
}
