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
#include "ui_menu.h"
#include "svmloader.h"
#include "svmclock.h"

#ifdef SIFTEO_SIMULATOR
#   include "system_mc.h"
#else
#   include "powermanager.h"
#endif

extern const uint16_t MenuBackground_data[];
extern const uint16_t IconQuit_data[];
extern const uint16_t IconBack_data[];
extern const uint16_t IconResume_data[];

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

    static const UIMenu::Item menuItems[] = {
        // Note: labels with an odd number of characters will center perfectly
        { IconBack_data,  "Game Menu" },
        { IconResume_data,   "Continue Game" },
        { IconQuit_data,     "Quit Game" },
    };

    enum MenuItems {
        kGameMenu,
        kContinue,
        kQuit,
    };

    if (!isPressed())
        return;

    if (SvmLoader::getRunLevel() == SvmLoader::RUNLEVEL_LAUNCHER)
        return;

    LOG(("Entering home button task\n"));
    SvmClock::pause();

    UICoordinator uic( Intrinsic::LZ(Tasks::AudioPull)  |
                       Intrinsic::LZ(Tasks::HomeButton) );

    UIMenu menu(uic, menuItems, arraysize(menuItems));

    SysTime::Ticks shutdownWarning = SysTime::ticks() + SysTime::sTicks(2);
    SysTime::Ticks shutdownDeadline = SysTime::ticks() + SysTime::sTicks(4);

    LED::set(LEDPatterns::paused, true);

    uint32_t buttonStates = ~0;
    int32_t scroll = 0;

    while (1) {
        uic.stippleCubes(uic.connectCubes());

        if (uic.pollForAttach()) {
            // New primary cube attached
            LOG(("Attached\n"));
            menu.init(kContinue);
        }

        menu.animate();

        // Menu done? Exit.
        if (menu.isDone()) {
            if (menu.getChosenItem() == kQuit)
                SvmLoader::exit();
            break;
        }

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

    LOG(("Leaving home button task\n"));
    SvmClock::resume();
}


} // namespace HomeButton
