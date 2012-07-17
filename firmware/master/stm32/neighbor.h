/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _NEIGHBOR_H
#define _NEIGHBOR_H

#include <stdint.h>
#include "gpio.h"
#include "hardware.h"
#include "hwtimer.h"

class Neighbor {
public:
    Neighbor(volatile TIM_t *txTimer, volatile TIM_t *rxTimer) :
        txData(0),
        txDataBuffer(0),
        txPeriodTimer(txTimer),
        rxPeriodTimer(rxTimer),
        txState(Idle)
    {}

    void init();

    void beginReceiving(uint8_t mask);
    void stopReceiving();

    void beginTransmitting(uint16_t data, uint8_t sideMask);
    void stopTransmitting();
    bool isTransmitting() {
        return txData > 0;
    }

    void txPeriodISR();
    void onRxPulse(uint8_t side);
    bool rxPeriodIsr(uint8_t &side, uint16_t &rxdata);

    static inline bool inIrqPending(uint8_t side) {
        return inPins[side].irqPending();
    }

private:
    /*
     * Timers are set to the neighbor transmission's bit width of 16us.
     * TIM3 and TIM5 are on APB1 which has a rate of 36MHz.
     *
     * Pulse duration is 2us.
     *
     * 2us  / (1 / 36000000) == 72 ticks
     * 12us / (1 / 36000000) == 432 ticks (this is the old bit period width, if you ever need it)
     * 16us / (1 / 36000000) == 576 ticks
     */
    static const unsigned PULSE_LEN_TICKS = 72;
    static const unsigned BIT_PERIOD_TICKS = 576;

    // number of bits to wait for during an rx sequence
    static const unsigned NUM_RX_BITS = 16;

    // number of bit periods to wait between transmissions
    static const unsigned NUM_TX_WAIT_PERIODS = 50;

#if (BOARD == BOARD_TEST_JIG)
    static const unsigned NUM_PINS = 4;
#else
    static const unsigned NUM_PINS = 2;
#endif

    void setDuty(uint16_t duty);

    volatile uint16_t txData;    // data in the process of being transmitted. if 0, we're done.
    uint16_t txDataBuffer;

    // sad to make these static, but no good way to init them other than
    // a) providing a default ctor for GPIOPin
    // b) enabling some c++0x business
    // not wanting to do either for now, so settle for a little brittleness
    static GPIOPin inPins[];
    static GPIOPin outPins[];
    static GPIOPin bufferPin;

    HwTimer txPeriodTimer;
    HwTimer rxPeriodTimer;

    union {
        uint16_t halfword;
        uint8_t bytes[2];
    } rxDataBuf;

    int8_t receivingSide;   // we only receive on one side at a time - which one?
    uint8_t rxBitCounter;   // how many bits have we shifted into rxDataBuffer?
    uint16_t txWaitPeriods; // how many bit periods to wait before beginning our next transmission

    enum RxState {
        WaitingForStart,    // waiting for next start bit, rxBitCounter == 0
        ReceivingData       // an rx sequence is in progress
    };

    enum TxState {
        Idle,
        BetweenTransmissions,
        Transmitting
    };

    RxState rxState;
    TxState txState;
};

#endif
