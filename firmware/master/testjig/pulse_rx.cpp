/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "pulse_rx.h"
#include "board.h"
#include "gpio.h"
#include "vectors.h"

namespace {
    static const GPIOPin pin = NBR_IN4_GPIO;
#define MAX_VALUE 65535
    static uint16_t pulseCounter;
}

void PulseRX::init()
{
    pin.irqInit();
    pin.irqSetFallingEdge();
    pin.irqDisable();
    pin.setControl(GPIOPin::IN_FLOAT);
}

void PulseRX::start()
{
    pulseCounter = 0;
    pin.irqEnable();
}

void PulseRX::stop()
{
    pin.irqDisable();
}

uint16_t PulseRX::count()
{
    return pulseCounter;
}

void PulseRX::pulseISR()
{
    // Saturate counter
    if (pulseCounter < MAX_VALUE)
        pulseCounter++;
}
