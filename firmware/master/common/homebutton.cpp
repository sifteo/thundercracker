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
    static const SysTime::Ticks holdDuration = SysTime::sTicks(4);
    static const SysTime::Ticks blinkPeriod = SysTime::msTicks(100);
    static SysTime::Ticks shutdownDeadline = 0;
    SysTime::Ticks now = SysTime::ticks();

    if (isPressed()) {
        /*
         * While button is held, poll for shutdown.
         */

        Tasks::trigger(Tasks::HomeButton);

        if (!shutdownDeadline) {
            // Require button to be held
            shutdownDeadline = now + holdDuration;
        }
        if (now >= shutdownDeadline) {
            // Actually shutting down now!
            shutdown();
        }

        // Start blinking as we get closer to the deadline
        SysTime::Ticks remaining = shutdownDeadline - now;
        if (remaining < holdDuration / 2) {
            bool blink = (remaining % blinkPeriod) < (blinkPeriod / 2);
            LED::set(blink ? LED::GREEN : LED::OFF);
        } else {
            LED::set(LED::GREEN);
        }

    } else {
        /*
         * Cancel shutdown
         */

        LED::set(LED::OFF);
        shutdownDeadline = 0;
    }
}


} // namespace HomeButton
