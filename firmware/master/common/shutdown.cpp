/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "shutdown.h"
#include "tasks.h"
#include "led.h"
#include "homebutton.h"
#include "radio.h"
#include "svmloader.h"
#include "cubeslots.h"
#include "cubeconnector.h"

#ifndef SIFTEO_SIMULATOR
#   include "powermanager.h"
#endif


ShutdownManager::ShutdownManager(uint32_t excludedTasks)
    : excludedTasks(excludedTasks)
{}

void ShutdownManager::shutdown()
{
    LOG(("SHUTDOWN: Beginning shutdown sequence\n"));

    // Make sure the home button is released before we go on
    while (HomeButton::isPressed())
        Tasks::idle(excludedTasks);

    // First round of shut down. We'll appear to be off.
    LED::set(NULL);
    CubeSlots::setCubeRange(0, 0);

    // We have plenty of time now. Clean up.
    housekeeping();

    // Wait until we've finished shutting down all cubes
    while (CubeSlots::sendShutdown & CubeSlots::sysConnected)
        Tasks::idle(excludedTasks);

    /*
     * If we're on battery power, now's a good time to sign off.
     */

    batteryPowerOff();

    /*
     * Otherwise, keep serving USB requests, but without any radio traffic.
     * Wait until the home button has been pressed again.
     */

    RadioManager::disableRadio();
    CubeSlots::disconnectCubes(CubeSlots::sysConnected);

    LOG(("SHUTDOWN: Entering USB-sleep state\n"));

    while (!HomeButton::isPressed())
        Tasks::idle(excludedTasks);

    /*
     * Back to life! From the user's perspective, this looks like a
     * fresh boot. However we don't actually want to reboot the system,
     * since that would unnecessarily cause us to drop off the USB.
     */

    batteryPowerOn();
    Tasks::cancel(Tasks::Pause);
    CubeConnector::enableReconnect();
    RadioManager::enableRadio();
    SvmLoader::execLauncher();

    LOG(("SHUTDOWN: Resuming from USB-sleep state\n"));
}

void ShutdownManager::housekeeping()
{
    /*
     * First make some more room if we can, by running the global garbage
     * collector until it can't find any more garbage. Then, consoldiate
     * this extra space into pre-erased blocks, as much as possible.
     */

    while (FlashLFS::collectGlobalGarbage()) {}

    FlashVolume::preEraseBlocks();
}

void ShutdownManager::batteryPowerOff()
{
    /*
     * On hardware only, and on battery power only, turn the system off.
     *
     * Has no effect in simulation.
     *
     * If we're on USB power this does not immediately turn us off,
     * but it ensures we will turn off when USB power disappears.
     * To make sure we don't worry about rail transition after this,
     * add PowerManager to our excluded tasks.
     */

    excludedTasks |= Intrinsic::LZ(Tasks::PowerManager);

    #ifndef SIFTEO_SIMULATOR
    return PowerManager::batteryPowerOff();
    #endif
}

void ShutdownManager::batteryPowerOn()
{
    excludedTasks &= ~Intrinsic::LZ(Tasks::PowerManager);

#ifndef SIFTEO_SIMULATOR
    return PowerManager::batteryPowerOn();
#endif
}
