/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _NEIGHBOR_H
#define _NEIGHBOR_H

#include <stdint.h>
#include "gpio.h"
#include "hwtimer.h"

class Neighbor {
public:
    Neighbor(volatile TIM_t *txTimer, volatile TIM_t *rxTimer) :
        txPeriodTimer(txTimer),
        rxPeriodTimer(rxTimer)
    {}
    void init();

    void beginReceiving();
    void beginTransmit(uint16_t data);
    bool isTransmitting() {
        return txData > 0;
    }

    void transmitNextBit();
    void onRxPulse(uint8_t side);
    void rxPeriodIsr();
    uint16_t getLastRxData();

private:
    // pwm duty specifying the duration of a tx pulse
    static const unsigned TX_ACTIVE_PULSE_DUTY = 25;
    // number of bits to wait for during an rx sequence
    static const unsigned NUM_RX_BITS = 16;

    void setDuty(uint16_t duty);
    void enablePwm();
    void disablePwm();

    uint16_t txData;    // data in the process of being transmitted. if 0, we're done.
    uint8_t input_bit_counter;

    // sad to make these static, but no good way to init them other than
    // a) providing a default ctor for GPIOPin
    // b) enabling some c++0x business
    // not wanting to do either for now, so settle for a little brittleness
    static GPIOPin inPins[];
    static GPIOPin outPins[];

    HwTimer txPeriodTimer;
    HwTimer rxPeriodTimer;

    int8_t receiving_side;
    uint16_t received_data_buffer;
    uint16_t lastRxData;

    enum RxState {
        Squelch,
        WaitingForNextBit,
        WaitingForStart
    };

    RxState rxState;
};

#endif
