/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "neighbor_rx.h"
#include "neighbor_tx.h"
#include "neighbor_protocol.h"
#include "board.h"
#include "bits.h"
#include "gpio.h"
#include "hwtimer.h"
#include "vectors.h"


namespace {

    static const GPIOPin pins[] = {
        NBR_IN1_GPIO,
        NBR_IN2_GPIO,
    #if (BOARD == BOARD_TEST_JIG)
        NBR_IN3_GPIO,
        NBR_IN4_GPIO,
    #endif
    };

    static union {
        uint16_t halfword;
        uint8_t bytes[2];
    } rxDataBuf;

    static int8_t receivingSide;   // we only receive on one side at a time - which one?
    static uint8_t rxBitCounter;   // how many bits have we shifted into rxDataBuffer?

    static enum RxState {
        WaitingForStart,    // waiting for next start bit, rxBitCounter == 0
        ReceivingData       // an rx sequence is in progress
    } rxState;

}

// config option
#define DISABLE_SQUELCH

void NeighborRX::init()
{
    #ifdef NBR_BUF_GPIO
        GPIOPin bufferPin = NBR_BUF_GPIO;
        bufferPin.setControl(GPIOPin::OUT_2MHZ);
    #endif

    HwTimer rxPeriodTimer(&NBR_RX_TIM);
    rxPeriodTimer.init(Neighbor::BIT_PERIOD_TICKS, 0);

    for (unsigned i = 0; i < arraysize(pins); ++i) {
        const GPIOPin &in = pins[i];
        in.irqInit();
        in.irqSetFallingEdge();
        in.irqDisable();
        in.setControl(GPIOPin::IN_FLOAT);
    }
}

void NeighborRX::start(unsigned mask)
{
    /*
     * Initialize state, and enable IRQs on input pins.
     * Bit period timer gets enabled after we receive a start bit.
     */

    #ifdef NBR_BUF_GPIO
        GPIOPin bufferPin = NBR_BUF_GPIO;
        bufferPin.setHigh();
    #endif

    HwTimer rxPeriodTimer(&NBR_RX_TIM);
    rxPeriodTimer.disableUpdateIsr();

    rxState = WaitingForStart;
    receivingSide = 0;  // just initialize to something safe
    rxBitCounter = 0;   // incremented at the end of each bit period

    for (unsigned i = 0; i < arraysize(pins); ++i) {
        NeighborTX::floatSide(i);

        const GPIOPin &in = pins[i];
        if (mask & (1 << i))
            in.irqEnable();
        else
            in.irqDisable();
    }
}

void NeighborRX::stop()
{
    /*
     * Disable IRQs for input pins and rx timer.
     */

    for (unsigned i = 0; i < arraysize(pins); ++i) {
        const GPIOPin &in = pins[i];
        in.irqDisable();
    }

    HwTimer rxPeriodTimer(&NBR_RX_TIM);
    rxPeriodTimer.disableUpdateIsr();

    #ifdef NBR_BUF_GPIO
        GPIOPin bufferPin = NBR_BUF_GPIO;
        bufferPin.setLow();
    #endif
}

void NeighborRX::pulseISR(unsigned side)
{
    /*
     * Called from within the EXTI ISR for one of our neighbor RX inputs.
     * No-op if the corresponding pin doesn't have an interrupt pending,
     * so it's okay to call if the actual interrupt source is unknown.
     *
     * OR a 1 into our rxDataBuffer for this bit period.
     *
     * NOTE: rxDataBuffer gets shifted at the bit period boundary: ISR_FN(NBR_RX_TIM)
     */

    const GPIOPin &in = pins[side];
    if (!in.irqPending())
        return;

    in.irqAcknowledge();

#ifndef DISABLE_SQUELCH
    NeighborTX::squelchSide(side);
#endif

    /*
     * Start of a new sequence?
     * Record which side we're listening to and disable the others.
     */
    if (rxState == WaitingForStart) {
        rxBitCounter = 0;
        rxDataBuf.halfword = 0;

        receivingSide = side;
        rxState = ReceivingData;

        /*
         * Sync our counter to this start bit, but wait as long as posible
         * to reset our counter, such that we're sampling
         * as far into the bit period as possible
         */
        HwTimer rxPeriodTimer(&NBR_RX_TIM);
        rxPeriodTimer.setCount(Neighbor::BIT_SAMPLE_PHASE);
        rxPeriodTimer.enableUpdateIsr();

    }

    // record the new bit that has arrived.
    if (int(side) == receivingSide) {
        rxDataBuf.halfword |= 0x1;
    }
}

IRQ_HANDLER ISR_FN(NBR_RX_TIM)()
{
    /*
     * Our bit period timer has expired during an RX operation.
     *
     * Allow our output pin to float, stopping the squelch on the tank.
     *
     * If this was the last bit in a transmission, verify that it's valid,
     * and reset our state. Otherwise, shift the value received during this bit
     * period along 'rxDataBuffer'
     */

    HwTimer rxPeriodTimer(&NBR_RX_TIM);
    uint32_t status = rxPeriodTimer.status();
    rxPeriodTimer.clearStatus();

    // update event
    if (status & 0x1) {
        #ifndef DISABLE_SQUELCH
        NeighborTX::floatSide(receivingSide);
        #endif

        // if we haven't gotten a start bit, nothing to do here
        if (rxState == WaitingForStart)
            return;

        rxBitCounter++;

        if (rxBitCounter < Neighbor::NUM_RX_BITS) {
            rxDataBuf.halfword <<= 1;
        } else {

            rxPeriodTimer.disableUpdateIsr();
            rxState = WaitingForStart;

            /*
             * The second byte of a successful message must match the complement
             * of its first byte, rotated right by 5.
             * Enforce that here before forwarding the message.
             */
            uint8_t checkbyte = ROR8(~rxDataBuf.bytes[1], 5);

            /*
             * Check the header explicitly - packets with a bogus header would
             * get rejected on the cubes since they would start listening on
             * the wrong side and the packet would be dropped.
             */
            bool headerOK = (rxDataBuf.bytes[1] & 0xe0) == 0xe0;

            if (headerOK && checkbyte == rxDataBuf.bytes[0])
                NeighborRX::callback(receivingSide, rxDataBuf.halfword);
        }
    }
}
