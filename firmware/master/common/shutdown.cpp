/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "shutdown.h"
#include "tasks.h"
#include "led.h"
#include "homebutton.h"
#include "radio.h"
#include "svmloader.h"
#include "cubeslots.h"
#include "cubeconnector.h"
#include "flash_preerase.h"
#include "idletimeout.h"

#ifndef SIFTEO_SIMULATOR
#   include "powermanager.h"
#endif


ShutdownManager::ShutdownManager(uint32_t excludedTasks)
    : excludedTasks(excludedTasks)
{}

void ShutdownManager::shutdown()
{
    LOG(("SHUTDOWN: Beginning shutdown sequence\n"));

    // First round of shut down. We'll appear to be off.
    LED::set(NULL);
    CubeSlots::setCubeRange(0, 0);

    // Make sure the home button is released before we go on
    while (HomeButton::isPressed())
        Tasks::idle(excludedTasks);

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

    while (!HomeButton::isPressed()) {
        Tasks::idle(excludedTasks);
        IdleTimeout::reset();   // don't let the watchdog kill us
    }

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
     * this extra space into pre-erased blocks.
     *
     * Give up as soon as possible if the home button is pressed again,
     * or if we start receiving USB traffic. This typically has a latency
     * of one flash block erasure.
     */

    while (!HomeButton::isPressed() && !Tasks::isPending(Tasks::UsbOUT)) {
        if (!FlashLFS::collectGlobalGarbage())
            break;
    }

    FlashBlockPreEraser bpe;
    while (!HomeButton::isPressed() && !Tasks::isPending(Tasks::UsbOUT)) {
        if (!bpe.next())
            break;
    }
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
