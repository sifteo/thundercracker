/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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

    NRF24L01::instance.init();
}

void Radio::setTxPower(TxPower pwr)
{
    NRF24L01::instance.setTxPower(pwr);
}

Radio::TxPower Radio::txPower()
{
    return NRF24L01::instance.txPower();
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

    /*
     * If no IRQs have happened since the last heartbeat, something
     * is wrong. Get exclusive access to the radio by disabling its
     * IRQ temporarily, and start a transmission.
     */

    if (radioWatchdog == NRF24L01::instance.irqCount) {
        NVIC.irqDisable(IVT.RF_EXTI_VEC);
        NRF24L01::instance.beginTransmitting();
        NVIC.irqEnable(IVT.RF_EXTI_VEC);
    }
    radioWatchdog = NRF24L01::instance.irqCount;
}
