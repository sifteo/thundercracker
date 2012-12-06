/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "gpio.h"
#include "board.h"
#include "vectors.h"
#include "pulse_rx.h"

PulseRX::PulseRX(GPIOPin p) :
        pin(p)
{
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
    pulseCount = 0;
    pin.irqEnable();
}

void PulseRX::stop()
{
    pin.irqDisable();
}

uint16_t PulseRX::count()
{
    return pulseCount;
}

void PulseRX::pulseISR()
{
    // Saturate counter
    if (pulseCount < maxCount)
        pulseCount++;
}
