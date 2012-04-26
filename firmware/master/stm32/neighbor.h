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
        txPeriodTimer(txTimer),
        rxPeriodTimer(rxTimer)
    {}

    void init();

    void beginReceiving();
    void stopReceiving();

    void beginTransmit(uint16_t data);
    bool isTransmitting() {
        return txData > 0;
    }

    void transmitNextBit();
    void onRxPulse(uint8_t side);
    void rxPeriodIsr();
    uint16_t getLastRxData() {
        return lastRxData;
    }

    inline static bool inIrqPending(uint8_t side) {
        return inPins[side].irqPending();
    }

private:
    // pwm duty specifying the duration of a tx pulse
    static const unsigned TX_ACTIVE_PULSE_DUTY = 25;
    // number of bits to wait for during an rx sequence
    static const unsigned NUM_RX_BITS = 16;

    void setDuty(uint16_t duty);
    void enablePwm();
    void disablePwm();

    uint16_t txData;    // data in the process of being transmitted. if 0, we're done.

    // sad to make these static, but no good way to init them other than
    // a) providing a default ctor for GPIOPin
    // b) enabling some c++0x business
    // not wanting to do either for now, so settle for a little brittleness
    static GPIOPin inPins[];
    static GPIOPin outPins[];

    HwTimer txPeriodTimer;
    HwTimer rxPeriodTimer;

    int8_t receivingSide;   // we only receive on one side at a time - which one?
    uint16_t rxDataBuffer;  // rx data in progress
    uint16_t lastRxData;    // last complete rx
    uint8_t rxBitCounter;   // how many bits have we shifted into rxDataBuffer?

    enum RxState {
        WaitingForStart,    // waiting for next start bit, rxBitCounter == 0
        Squelch,            //
        WaitingForNextBit
    };

    RxState rxState;
};

#endif
