/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "neighbor.h"

void Neighbor::init()
{
    nbr_out1.setControl(GPIOPin::OUT_ALT_50MHZ);
    nbr_out2.setControl(GPIOPin::OUT_ALT_50MHZ);
    nbr_out3.setControl(GPIOPin::OUT_ALT_50MHZ);
    nbr_out4.setControl(GPIOPin::OUT_ALT_50MHZ);

    nbr_in1.irqInit();
    nbr_in1.irqSetFallingEdge();
    nbr_in1.irqDisable();

    txPeriodTimer.init(625, 4);

    rxPeriodTimer.init(900, 0);

    txPeriodTimer.configureChannelAsOutput(1, HwTimer::ActiveHigh, HwTimer::Pwm1);
    txPeriodTimer.configureChannelAsOutput(2, HwTimer::ActiveHigh, HwTimer::Pwm1);
    txPeriodTimer.configureChannelAsOutput(3, HwTimer::ActiveHigh, HwTimer::Pwm1);
    txPeriodTimer.configureChannelAsOutput(4, HwTimer::ActiveHigh, HwTimer::Pwm1);
}

void Neighbor::enablePwm()
{
    nbr_out1.setControl(GPIOPin::OUT_ALT_50MHZ);
    nbr_out2.setControl(GPIOPin::OUT_ALT_50MHZ);
    nbr_out3.setControl(GPIOPin::OUT_ALT_50MHZ);
    nbr_out4.setControl(GPIOPin::OUT_ALT_50MHZ);

    txPeriodTimer.enableChannel(1);
    txPeriodTimer.enableChannel(2);
    txPeriodTimer.enableChannel(3);
    txPeriodTimer.enableChannel(4);
}

void Neighbor::setDuty(uint16_t duty)
{
    txPeriodTimer.setDuty(1, duty);
    txPeriodTimer.setDuty(2, duty);
    txPeriodTimer.setDuty(3, duty);
    txPeriodTimer.setDuty(4, duty);
}

void Neighbor::disablePwm()
{
    txPeriodTimer.disableChannel(1);
    txPeriodTimer.disableChannel(2);
    txPeriodTimer.disableChannel(3);
    txPeriodTimer.disableChannel(4);

    nbr_out1.setControl(GPIOPin::IN_FLOAT);
    nbr_out2.setControl(GPIOPin::IN_FLOAT);
    nbr_out3.setControl(GPIOPin::IN_FLOAT);
    nbr_out4.setControl(GPIOPin::IN_FLOAT);
}

/*
 * Begin the transmission of 'data'.
 */
void Neighbor::beginTransmit(uint16_t data)
{
    txData = data;
    enablePwm();
    transmitNextBit();
}

/*
 * Called when the PWM timer has reached the end of its counter, ie the end of a
 * bit period.
 */
void Neighbor::transmitNextBit()
{
    // if there are no more bits to send, we're done
    if (!txData)  {
        disablePwm();
        return;
    }

    // set the duty for this bit period
    if (txData & 1)
        setDuty(TX_ACTIVE_PULSE_DUTY);
    else
        setDuty(0);

    txData >>= 1;
}


void Neighbor::beginReceiving()
{
    receiving_side = -1;
    nbr_in1.setControl(GPIOPin::IN_FLOAT);
    nbr_in1.irqEnable();
}

/*
 * Called from within the EXTI ISR for one of our neighbor RX inputs.
 */
void Neighbor::onRxPulse(uint8_t side)
{
    if (side == 0) {
        nbr_in1.irqAcknowledge();

        nbr_out1.setControl(GPIOPin::OUT_2MHZ);
        nbr_out1.setLow();

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

    if (side == 1)  {
        nbr_in2.irqAcknowledge();
    }

    if (side == 2)  {
        nbr_in3.irqAcknowledge();
    }

    if (side == 3)  {
        nbr_in4.irqAcknowledge();
    }
}

/*
 * Our bit period timer has expired during an RX operation.
 */
void Neighbor::rxPeriodIsr()
{
    nbr_out1.setControl(GPIOPin::IN_FLOAT);

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
