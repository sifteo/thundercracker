/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "neighbor_tx.h"
#include "neighbor_protocol.h"
#include "board.h"
#include "bits.h"
#include "gpio.h"
#include "hwtimer.h"
#include "vectors.h"
#include "batterylevel.h"

namespace {

    static const GPIOPin pins[] = {
        NBR_OUT1_GPIO,
        NBR_OUT2_GPIO,
    #if (BOARD == BOARD_TEST_JIG)
        NBR_OUT3_GPIO,
        NBR_OUT4_GPIO
    #endif
    };

    static void setDuty(unsigned duty)
    {
        HwTimer txPeriodTimer(&NBR_TX_TIM);
        for (unsigned i = 0; i < arraysize(pins); ++i)
            txPeriodTimer.setDuty(NBR_TX_TIM_CH + i, duty);
    }

    static uint16_t txData;         // Shift register for transmission in progress, 0 if done
    static uint16_t txDataBuffer;   // Current packet we're transmitting
    static bool paused = false;

    static enum TxState {
        Idle,
        BetweenTransmissions,
        Transmitting
    } txState;

}

void NeighborTX::init()
{
    HwTimer txPeriodTimer(&NBR_TX_TIM);
    txPeriodTimer.init(Neighbor::BIT_PERIOD_TICKS, 0);

    for (unsigned i = 0; i < arraysize(pins); ++i) {
        unsigned channel = NBR_TX_TIM_CH + i;
        txPeriodTimer.configureChannelAsOutput(channel, HwTimer::ActiveHigh, HwTimer::Pwm1);
        pins[i].setControl(GPIOPin::IN_PULL);
        txPeriodTimer.enableChannel(channel);
    }
}

void NeighborTX::start(unsigned data, unsigned sideMask)
{
    /*
     * Begin the transmission of 'data'.
     *
     * Continue transmitting 'data' on a regular basis, until stopTransmitting()
     * is invoked. We wait NUM_TX_WAIT_PERIODS bit periods between transmissions.
     *
     * In the event that a transmission is ongoing, 'data' is buffered
     * until the start of the next transmission. sideMask is applied immediately.
     *
     * While we're paused, transmission has stopped but we'll still accept updates
     * to our txdata and sideMask config.
     */

    txDataBuffer = data;

    // configure pins according to sideMask
    for (unsigned i = 0; i < arraysize(pins); ++i) {
        GPIOPin::Control c = (sideMask & (1 << i)) ? GPIOPin::OUT_ALT_50MHZ : GPIOPin::IN_PULL;
        pins[i].setControl(c);
    }

    if (paused)
        return;

    // starting from idle? reinit state machine
    if (txState == Idle) {
        txData = txDataBuffer;
        txState = Transmitting;

        HwTimer txPeriodTimer(&NBR_TX_TIM);
        txPeriodTimer.enableUpdateIsr();

        // transmission will begin at the next txPeriodISR
    }
}

void NeighborTX::stop()
{
    HwTimer txPeriodTimer(&NBR_TX_TIM);
    txPeriodTimer.disableUpdateIsr();

    setDuty(0);

    for (unsigned i = 0; i < arraysize(pins); ++i)
        pins[i].setControl(GPIOPin::IN_PULL);

    txState = Idle;
}

void NeighborTX::pause()
{
    paused = true;
    NeighborTX::stop();
}

void NeighborTX::resume()
{
    paused = false;
    NeighborTX::start(txDataBuffer, ~0);
}

IRQ_HANDLER ISR_FN(NBR_TX_TIM)()
{
    /*
     * Called when the PWM timer has reached the end of its counter, ie the end of a
     * bit period.
     *
     * The duty value is loaded into the PWM device at the update counter event.
     *
     * We might be in the middle of transmitting some data, or we might be in our
     * waiting period between transmissions. Move along accordingly.
     */

    HwTimer txPeriodTimer(&NBR_TX_TIM);
    uint16_t status = txPeriodTimer.status();
    txPeriodTimer.clearStatus();

    // update event
    if (status & 0x1) {

        switch (txState) {

        case Transmitting:
            // was the previous bit our last one?
            if (txData == 0) {
                /*
                 * Between transmissions, we give battery measurement an
                 * opportunity to take a sample.
                 */

                setDuty(0);
                txPeriodTimer.setPeriod(Neighbor::BIT_PERIOD_TICKS, Neighbor::NUM_TX_WAIT_PERIODS);
                txState = BetweenTransmissions;
                #if BOARD != BOARD_TEST_JIG && BOARD != BOARD_MASTER_REV3
                BatteryLevel::beginCapture();
                #endif
            } else {
                // send data out big endian
                setDuty((txData & 0x8000) ? Neighbor::PULSE_LEN_TICKS : 0);
                txData <<= 1;
            }
            break;

        case BetweenTransmissions:
            // Load data from our buffer and update our state.
            // Go back to our usual 1x prescaler.

            txData = txDataBuffer;
            txState = Transmitting;
            txPeriodTimer.setPeriod(Neighbor::BIT_PERIOD_TICKS, 0);
            break;

        case Idle:
            // should never be called
            break;
        }
    }

    if (status & (1 << BATT_LVL_CHAN)) {
        BatteryLevel::captureIsr();
    }
}

void NeighborTX::floatSide(unsigned side)
{
#ifdef NBR_BUF_GPIO
    GPIOPin bufferPin = NBR_BUF_GPIO;
    bufferPin.setHigh();
#endif

    pins[side].setControl(GPIOPin::IN_FLOAT);
}

void NeighborTX::squelchSide(unsigned side)
{
    const GPIOPin &out = pins[side];
    out.setControl(GPIOPin::OUT_2MHZ);
    out.setLow();

#ifdef NBR_BUF_GPIO
    GPIOPin bufferPin = NBR_BUF_GPIO;
    bufferPin.setLow();
#endif
}
