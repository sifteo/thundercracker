/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "neighbor.h"
#include "board.h"

#if (BOARD == BOARD_TEST_JIG)
#include "testjig.h"
#endif

GPIOPin Neighbor::inPins[4] = {
    NBR_IN1_GPIO,
    NBR_IN2_GPIO,
    NBR_IN3_GPIO,
    NBR_IN4_GPIO,

};

GPIOPin Neighbor::outPins[4] = {
    NBR_OUT1_GPIO,
    NBR_OUT2_GPIO,
    NBR_OUT3_GPIO,
    NBR_OUT4_GPIO
};

void Neighbor::init()
{
    txData = 0;

    for (unsigned i = 0; i < 4; ++i) {
        outPins[i].setControl(GPIOPin::IN_FLOAT);

        GPIOPin &in = inPins[i];
        in.irqInit();
        in.irqSetFallingEdge();
        in.irqDisable();
    }

    txPeriodTimer.init(625, 4);
    rxPeriodTimer.init(820, 0); // ~ 24 us

    for (unsigned i = 1; i < 5; ++i)
        txPeriodTimer.configureChannelAsOutput(i, HwTimer::ActiveHigh, HwTimer::Pwm1);
}

void Neighbor::enablePwm()
{
    for (unsigned i = 0; i < 4; ++i)
        outPins[i].setControl(GPIOPin::OUT_ALT_50MHZ);

    for (unsigned i = 1; i < 5; ++i)
        txPeriodTimer.enableChannel(i);

    txPeriodTimer.enableUpdateIsr();
}

void Neighbor::setDuty(uint16_t duty)
{
    for (unsigned i = 1; i < 5; ++i)
        txPeriodTimer.setDuty(i, duty);
}

void Neighbor::disablePwm()
{
    for (unsigned i = 1; i < 5; ++i)
        txPeriodTimer.disableChannel(i);

    for (unsigned i = 0; i < 4; ++i)
        outPins[i].setControl(GPIOPin::IN_FLOAT);
}

/*
 * Begin the transmission of 'data'.
 */
void Neighbor::beginTransmit(uint16_t data)
{
    if (isTransmitting())
        return;

    txData = data;
    transmitNextBit();  // ensure duty is set before enabling pwm channels
    enablePwm();
}

/*
 * Called when the PWM timer has reached the end of its counter, ie the end of a
 * bit period.
 */
void Neighbor::transmitNextBit()
{
    // if there are no more bits to send, we're done
    if (!isTransmitting()) {
        disablePwm();
        return;
    }

    // set the duty for this bit period
    setDuty((txData & 1) ? TX_ACTIVE_PULSE_DUTY : 0);
    txData >>= 1;
}

/*
 * Initialize state, and enable IRQs on input pins & rx timer.
 */
void Neighbor::beginReceiving()
{
    rxState = WaitingForStart;
    receivingSide = 0;  // just initialize to something safe

    for (unsigned i = 0; i < 4; ++i) {
        GPIOPin &in = inPins[i];
        in.setControl(GPIOPin::IN_FLOAT);
        in.irqEnable();
    }

    rxPeriodTimer.enableUpdateIsr();
}

/*
 * Disable IRQs for input pins and rx timer.
 */
void Neighbor::stopReceiving()
{
    for (unsigned i = 0; i < 4; ++i)
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
        rxBitCounter = 0;   // this gets incremented at the end of the bit period
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
void Neighbor::rxPeriodIsr()
{
    outPins[receivingSide].setControl(GPIOPin::IN_FLOAT);

    // if we haven't gotten a start bit, nothing to do here
    if (rxState == WaitingForStart)
        return;

    rxBitCounter++;

    if (rxBitCounter < NUM_RX_BITS) {
        rxDataBuffer <<= 1;
    } else {
        /*
         * The second byte of a successful message must match the complement
         * of its first byte. Enforce that here before forwarding the message.
         */
        int8_t lsb = rxDataBuffer & 0xff;
        int8_t msb = (rxDataBuffer >> 8) & 0xff;
        if (lsb == ~msb) {

#if (BOARD == BOARD_TEST_JIG)
            TestJig::onNeighborMsgRx(receivingSide, rxDataBuffer);
#endif

        }

        rxState = WaitingForStart;
        rxDataBuffer = 0;
    }
}
