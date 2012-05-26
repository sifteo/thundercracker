/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "led.h"
#include "gpio.h"
#include "board.h"
#include "macros.h"


void LED::set(Color c)
{
    GPIOPin red = LED_RED_GPIO;
    GPIOPin green = LED_GREEN_GPIO;

    red.setControl(GPIOPin::OUT_2MHZ);
    green.setControl(GPIOPin::OUT_2MHZ);

    if (c & RED)
        red.setLow();
    else
        red.setHigh();

    if (c & GREEN)
        green.setLow();
    else
        green.setHigh();
}
