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

/*
 * High-level radio interface. This is the glue between our
 * cross-platform code and the low-level nRF radio driver.
 *
 * We implement a radio watchdog here, so that if the radio ever
 * drops a transaction on the floor (hardware bug, software bug,
 * electrical noise) we can restart our transmit/ack loop.
 */

#include "vectors.h"
#include "board.h"
#include "systime.h"
#include "radio.h"
#include "nrf24l01.h"

static uint8_t radioStartup = 0;
static uint32_t radioWatchdog = 0;
static SysTime::Ticks reinitPowerOnDelay;


void Radio::init()
{
    /*
     * NOTE: the radio has 2 100ms delays on a power on reset: one before
     * we can talk to it at all, and one before we can start transmitting.
     *
     * Here, we block on the first delay. Note that we're measuring total
     * system uptime, so any initialization we can perform before this
     * point is "free" as far as boot time is concerned.
     *
     * The second delay is implied by the fact that we need to wait for
     * a Radio::heartbeat() before we'll actually ask the radio to begin
     * transmitting.
     */

    while (SysTime::ticks() < SysTime::msTicks(110));
    radioStartup = 1;
    radioWatchdog = 0;
    reinitPowerOnDelay = 0;

#ifdef USE_NRF24L01
    NRF24L01::instance.init();
#endif
    RadioManager::enableRadio();
}

void Radio::onTransitionToUsbPower()
{
    /*
     * Work around a bit of a hardware bug.
     *
     * When we're running on batteries, and transition to USB power, the radio
     * can lose power briefly. Since we need to wait for the radio's first
     * 100ms power on, start that count running as soon as possible here.
     *
     * This signals the watchdog to reinit the radio, below.
     */

    reinitPowerOnDelay = SysTime::ticks() + SysTime::msTicks(110);
}

void Radio::reinit()
{
    /*
     * As with init() above, we need to accomodate the radio's power on delays,
     * and then schedule transmission to begin once we've seen enough heartbeats
     * to ensure that the radio's 2nd power on delay has been satisfied.
     *
     * We need the RadioManager to count the previous abandoned packet as
     * timed out - it likely did not get where it was going, but more
     * importantly, if we treated it as anything else, we could be out of sync
     * with the cube we were transmitting to.
     */

    while (SysTime::ticks() < reinitPowerOnDelay);
    radioStartup = 1;

    RadioManager::timeout();
#ifdef USE_NRF24L01
    NRF24L01::instance.init();
#endif
}

void Radio::heartbeat()
{
    /*
     * Runs on the main thread, at Tasks::HEARTBEAT_HZ.
     * If we haven't transmitted anything since the last
     * heartbeat, restart transmission.
     *
     * In the interest of designing for better testing coverage,
     * this same process is also used to start the transmitter for
     * the first time, after our requisite startup delay.
     */

    if (radioStartup == 0) {
        // Not yet init()'ed
        return;
    } else if (radioStartup <= 2) {
        // Wait until we're sure at least 100ms has elapsed
        radioStartup++;
        return;
    }

    if (!RadioManager::isRadioEnabled()) {
        return;
    }

    /*
     * If no IRQs have happened since the last heartbeat, something
     * is wrong. Get exclusive access to the radio by disabling its
     * IRQ temporarily, and start a transmission.
     *
     * XXX: possibly reorganize this as support for the 51822
     *      radio continues to take shape.
     */

#ifdef USE_NRF24L01

    if (radioWatchdog == NRF24L01::instance.irqCount) {
        NVIC.irqDisable(IVT.RF_EXTI_VEC);

        if (reinitPowerOnDelay) {
            reinit();
            reinitPowerOnDelay = 0;
        } else {
            NRF24L01::instance.beginTransmitting();
        }

        NVIC.irqEnable(IVT.RF_EXTI_VEC);
    }

    radioWatchdog = NRF24L01::instance.irqCount;

#endif
}
