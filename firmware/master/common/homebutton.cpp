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

    LOG(("Entering home button task\n"));

    UICoordinator uic( Intrinsic::LZ(Tasks::AudioPull)  |
                       Intrinsic::LZ(Tasks::HomeButton) );

    SysTime::Ticks shutdownWarning = SysTime::ticks() + SysTime::sTicks(2);
    SysTime::Ticks shutdownDeadline = SysTime::ticks() + SysTime::sTicks(4);

    LED::set(LEDPatterns::paused, true);

    while (isPressed()) {

        uic.stippleCubes(uic.connectCubes());

        if (uic.pollForAttach()) {
            // New primary cube attached
            LOG(("Attached\n"));

            VRAM::poke(uic.avb.vbuf, 0, _SYS_TILE77('x' - ' '));
        }

        uic.paint();

        if (SysTime::ticks() > shutdownWarning)
            LED::set(LEDPatterns::shutdown);

        if (SysTime::ticks() > shutdownDeadline)
            shutdown();
    }

    uic.restoreCubes(uic.uiConnected);
    LED::set(LEDPatterns::idle);

    LOG(("Leaving home button task\n"));
}


} // namespace HomeButton
