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

#ifdef SIFTEO_SIMULATOR
#   include "system_mc.h"
#else
#   include "powermanager.h"
#endif


namespace HomeButton {


void shutdown()
{
    #ifdef SIFTEO_SIMULATOR
        SystemMC::exit(false);
    #else
        PowerManager::shutdown();
    #endif
    while (1);
}


void task()
{
    /*
     * XXX: This is all just a testbed currently, none of this is intended to be final.
     */

    if (!isPressed())
        return;

    if (SvmLoader::getRunLevel() == SvmLoader::RUNLEVEL_LAUNCHER)
        return;

    SvmClock::pause();
    LED::set(LEDPatterns::paused, true);

    UICoordinator uic( Intrinsic::LZ(Tasks::AudioPull)  |
                       Intrinsic::LZ(Tasks::HomeButton) );

    UIPause ui(uic);

    SysTime::Ticks shutdownWarning = SysTime::ticks() + SysTime::sTicks(2);
    SysTime::Ticks shutdownDeadline = SysTime::ticks() + SysTime::sTicks(4);

    uint32_t buttonStates = ~0;

    while (1) {
        uic.stippleCubes(uic.connectCubes());

        if (uic.pollForAttach())
            ui.init();

        ui.animate();

        // Menu done? Exit.
        if (ui.isDone())
            break;

        // Another release/press/release on home button causes us to exit immediately
        uint32_t button = isPressed();
        if ((buttonStates ^ button) & 1) {
            buttonStates = (buttonStates << 1) | button;
            if ((buttonStates & 0x0F) == 0x0A)
                break;
        }
        uic.paint();
    }

    uic.restoreCubes(uic.uiConnected);
    LED::set(LEDPatterns::idle);
    SvmClock::resume();
    ui.takeAction();
}


} // namespace HomeButton
