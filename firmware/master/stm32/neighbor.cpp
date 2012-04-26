/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "neighbor.h"
#include "board.h"

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

    for (unsigned i = 0; i < 4; ++i)
        outPins[i].setControl(GPIOPin::OUT_ALT_50MHZ);

    GPIOPin &in1 = inPins[0];
    in1.irqInit();
    in1.irqSetFallingEdge();
    in1.irqDisable();

    // this is currently about 87 us
    txPeriodTimer.init(625, 4);
    rxPeriodTimer.init(900, 0);

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


void Neighbor::beginReceiving()
{
    receiving_side = -1;

    GPIOPin &in1 = inPins[0];
    in1.setControl(GPIOPin::IN_FLOAT);
    in1.irqEnable();
}

/*
 * Called from within the EXTI ISR for one of our neighbor RX inputs.
 */
void Neighbor::onRxPulse(uint8_t side)
{
    inPins[side].irqAcknowledge();

    if (side == 0) {

        GPIOPin &out1 = outPins[0];
        out1.setControl(GPIOPin::OUT_2MHZ);
        out1.setLow();

        switch (rxState) {
        case WaitingForStart:
            received_data_buffer = 1;  //set bit 0 to 1
            input_bit_counter = 1;
            rxState = Squelch;
            break;

        case WaitingForNextBit:
        case Squelch:
            received_data_buffer |= (1 << input_bit_counter);
            input_bit_counter++;
            rxState = Squelch;
            break;
        }
    }
}

/*
 * Our bit period timer has expired during an RX operation.
 */
void Neighbor::rxPeriodIsr()
{
    outPins[0].setControl(GPIOPin::IN_FLOAT);

    switch (rxState) {
    case Squelch:
        rxState = WaitingForNextBit;
        break;

    case WaitingForNextBit:
        // input bit must be a zero
        input_bit_counter++;
        rxState = Squelch;
        if (input_bit_counter >= NUM_RX_BITS) {
            lastRxData = received_data_buffer;
            rxState = WaitingForStart;
            received_data_buffer = 0;
        }
        break;

    default:
        break;
    }
}

uint16_t Neighbor::getLastRxData()
{
    uint16_t val = lastRxData;
    lastRxData = 0;
    return val;
}
