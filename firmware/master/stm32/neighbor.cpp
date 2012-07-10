/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "neighbor.h"
#include "board.h"

GPIOPin Neighbor::inPins[] = {
    NBR_IN1_GPIO,
    NBR_IN2_GPIO,
#if (BOARD == BOARD_TEST_JIG)
    NBR_IN3_GPIO,
    NBR_IN4_GPIO,
#endif
};

GPIOPin Neighbor::outPins[] = {
    NBR_OUT1_GPIO,
    NBR_OUT2_GPIO,
#if (BOARD == BOARD_TEST_JIG)
    NBR_OUT3_GPIO,
    NBR_OUT4_GPIO
#endif
};

void Neighbor::init()
{
    txPeriodTimer.init(BIT_PERIOD_TICKS, 0);
    rxPeriodTimer.init(BIT_PERIOD_TICKS, 0);

    for (unsigned i = 0; i < NUM_PINS; ++i) {

        txPeriodTimer.configureChannelAsOutput(i + 1, HwTimer::ActiveHigh, HwTimer::Pwm1);
        outPins[i].setControl(GPIOPin::IN_PULL);
        txPeriodTimer.enableChannel(i + 1);

        GPIOPin &in = inPins[i];
        in.irqInit();
        in.irqSetFallingEdge();
        in.irqDisable();
    }
}

void Neighbor::setDuty(uint16_t duty)
{
    for (unsigned i = 1; i < NUM_PINS + 1; ++i)
        txPeriodTimer.setDuty(i, duty);
}

/*
 * Begin the transmission of 'data'.
 *
 * Continue transmitting 'data' on a regular basis, until stopTransmitting()
 * is invoked. We wait NUM_TX_WAIT_PERIODS bit periods between transmissions.
 *
 * In the event that a transmission is ongoing, 'data' is buffered
 * until the start of the next transmission. sideMask is applied immediately.
 */
void Neighbor::beginTransmitting(uint16_t data, uint8_t sideMask)
{
    txDataBuffer = data;

    // configure pins according to sideMask
    for (unsigned i = 0; i < NUM_PINS; ++i) {
        GPIOPin::Control c = (sideMask & (1 << i)) ? GPIOPin::OUT_ALT_50MHZ : GPIOPin::IN_PULL;
        outPins[i].setControl(c);
    }

    // starting from idle? reinit state machine
    if (txState == Idle) {
        data = txDataBuffer;
        txState = Transmitting;
        txPeriodTimer.enableUpdateIsr();
        // transmission will begin at the next txPeriodISR
    }
}

void Neighbor::stopTransmitting()
{
    txPeriodTimer.disableUpdateIsr();
    setDuty(0);
    for (unsigned i = 0; i < NUM_PINS; ++i)
        outPins[i].setControl(GPIOPin::IN_PULL);
    txState = Idle;
}

/*
 * Called when the PWM timer has reached the end of its counter, ie the end of a
 * bit period.
 *
 * The duty value is loaded into the PWM device at the update counter event.
 *
 * We might be in the middle of transmitting some data, or we might be in our
 * waiting period between transmissions. Move along accordingly.
 */
void Neighbor::txPeriodISR()
{
    uint16_t status = txPeriodTimer.status();
    txPeriodTimer.clearStatus();

    // update event
    if (status & 0x1) {

        switch (txState) {

        case Transmitting:
            // was the previous bit our last one?
            if (txData == 0) {
                setDuty(0);
                txWaitPeriods = NUM_TX_WAIT_PERIODS;
                txState = BetweenTransmissions;
            } else {
                // send data out big endian
                setDuty((txData & 0x8000) ? PULSE_LEN_TICKS : 0);
                txData <<= 1;
            }
            break;

        case BetweenTransmissions:
            if (--txWaitPeriods == 0) {
                // load data from our buffer and update our state
                txData = txDataBuffer;
                txState = Transmitting;
            }
            break;

        case Idle:
            // should never be called
            break;
        }
    }
}

/*
 * Initialize state, and enable IRQs on input pins & rx timer.
 */
void Neighbor::beginReceiving(uint8_t mask)
{
    mask &= SIDEMASK;

    rxState = WaitingForStart;
    receivingSide = 0;  // just initialize to something safe
    rxBitCounter = 0;   // incremented at the end of each bit period

    for (unsigned i = 0; mask != 0; ++i, mask >>= 1) {
        if (mask & 0x1) {
            GPIOPin &in = inPins[i];
            in.setControl(GPIOPin::IN_FLOAT);
            in.irqEnable();
        }
    }

    rxPeriodTimer.enableUpdateIsr();
}

/*
 * Disable IRQs for input pins and rx timer.
 */
void Neighbor::stopReceiving()
{
    for (unsigned i = 0; i < NUM_PINS; ++i)
        inPins[i].irqDisable();

    rxPeriodTimer.disableUpdateIsr();
}

/*
 * Called from within the EXTI ISR for one of our neighbor RX inputs.
 * OR a 1 into our rxDataBuffer for this bit period.
 *
 * NOTE: rxDataBuffer gets shifted at the bit period boundary (rxPeriodIsr())
 */
void Neighbor::onRxPulse(uint8_t side)
{
    inPins[side].irqAcknowledge();

    // TODO: may need to time the squelch more precisely to ensure we don't
    // worsen the ringing
    GPIOPin &out = outPins[side];
    out.setControl(GPIOPin::OUT_50MHZ);
    out.setLow();

    /*
     * Start of a new sequence?
     * Record which side we're listening to and disable the others.
     */
    if (rxState == WaitingForStart) {

        receivingSide = side;
        rxDataBuffer = 1;   // init bit 0 to 1 - we're here because we received a pulse after all
        rxState = ReceivingData;

        /*
         * Sync our counter to this start bit, but wait as long as posible
         * to reset our counter, such that we're sampling
         * as far into the bit period as possible
         **/
        rxPeriodTimer.setCount(0);

    } else {
        // Otherwise, record the new bit that has arrived.
        if (side == receivingSide)
            rxDataBuffer |= 0x1;
    }
}

/*
 * Our bit period timer has expired during an RX operation.
 *
 * Allow our output pin to float, stopping the squelch on the tank.
 *
 * If this was the last bit in a transmission, verify that it's valid,
 * and reset our state. Otherwise, shift the value received during this bit
 * period along 'rxDataBuffer'
 */
bool Neighbor::rxPeriodIsr(uint16_t *side, uint16_t *rxData)
{
    outPins[receivingSide].setControl(GPIOPin::IN_FLOAT);

    // if we haven't gotten a start bit, nothing to do here
    if (rxState == WaitingForStart)
        return false;

    rxBitCounter++;

    if (rxBitCounter < NUM_RX_BITS) {
        rxDataBuffer <<= 1;
    } else {

        rxState = WaitingForStart;

        /*
         * The second byte of a successful message must match the complement
         * of its first byte. Enforce that here before forwarding the message.
         */
        int8_t lsb = rxDataBuffer & 0xff;
        int8_t msb = (rxDataBuffer >> 8) & 0xff;
        if (lsb == ~msb) {
            *side = receivingSide;
            *rxData = rxDataBuffer;
            return true;
        }
    }

    return false;
}
