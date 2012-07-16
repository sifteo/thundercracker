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

GPIOPin Neighbor::bufferPin = NBR_BUF_GPIO;

void Neighbor::init()
{
    bufferPin.setControl(GPIOPin::OUT_2MHZ);

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
        in.setControl(GPIOPin::IN_FLOAT);
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
 * Initialize state, and enable IRQs on input pins.
 * Bit period timer gets enabled after we receive a start bit.
 */
void Neighbor::beginReceiving(uint8_t mask)
{
    bufferPin.setHigh();

    rxPeriodTimer.disableUpdateIsr();
    rxState = WaitingForStart;
    receivingSide = 0;  // just initialize to something safe
    rxBitCounter = 0;   // incremented at the end of each bit period

    for (unsigned i = 0; i < NUM_PINS; ++i) {

        outPins[i].setControl(GPIOPin::IN_FLOAT);

        GPIOPin &in = inPins[i];
        if (mask & (1 << i))
            in.irqEnable();
        else
            in.irqDisable();
    }
}

/*
 * Disable IRQs for input pins and rx timer.
 */
void Neighbor::stopReceiving()
{
    for (unsigned i = 0; i < NUM_PINS; ++i)
        inPins[i].irqDisable();

    rxPeriodTimer.disableUpdateIsr();
    bufferPin.setLow();
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

    GPIOPin &out = outPins[side];
    out.setControl(GPIOPin::OUT_2MHZ);
    out.setLow();

    /*
     * Start of a new sequence?
     * Record which side we're listening to and disable the others.
     */
    if (rxState == WaitingForStart) {
        /*
         * Treat this as though we're at the end of our first bit period,
         * during which we received a pulse. We're now just starting our
         * second bit period to listen for the next pulse.
         */
        rxBitCounter = 1;
        rxDataBuf.halfword = 1 << 1;     // init bit 0 to 1 - we're here because we received a pulse after all

        receivingSide = side;
        rxState = ReceivingData;

        /*
         * Sync our counter to this start bit, but wait as long as posible
         * to reset our counter, such that we're sampling
         * as far into the bit period as possible
         */
        rxPeriodTimer.setCount(0);
        rxPeriodTimer.enableUpdateIsr();

    } else {
        // Otherwise, record the new bit that has arrived.
        if (side == receivingSide) {
            rxDataBuf.halfword |= 0x1;
        }
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
bool Neighbor::rxPeriodIsr(uint8_t &side, uint16_t &rxdata)
{
    uint32_t status = rxPeriodTimer.status();
    rxPeriodTimer.clearStatus();

    // update event
    if (status & 0x1) {
        outPins[receivingSide].setControl(GPIOPin::IN_FLOAT);

        // if we haven't gotten a start bit, nothing to do here
        if (rxState == WaitingForStart)
            return false;

        rxBitCounter++;

        if (rxBitCounter < NUM_RX_BITS) {
            rxDataBuf.halfword <<= 1;
        } else {

            rxPeriodTimer.disableUpdateIsr();
            rxState = WaitingForStart;

            /*
             * The second byte of a successful message must match the complement
             * of its first byte, rotated right by 5.
             * Enforce that here before forwarding the message.
             */
            uint8_t checkbyte = ror8(~rxDataBuf.bytes[1], 5);

            /*
             * Check the header explicitly - packets with a bogus header would
             * get rejected on the cubes since they would start listening on
             * the wrong side and the packet would be dropped.
             */
            bool headerOK = (rxDataBuf.bytes[1] & 0xe0) == 0xe0;

            if (headerOK && checkbyte == rxDataBuf.bytes[0]) {
                side = receivingSide;
                rxdata = rxDataBuf.halfword;
                return true;
            }
        }
    }

    return false;
}
