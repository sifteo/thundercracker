/*
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */


#ifndef BUTTON_H_
#define BUTTON_H_

#include "gpio.h"

class Button
{
public:
    Button(GPIOPin pin) : irq(pin)
    {}

    static void enablePushButton();

    void init();
    void isr();

private:
    GPIOPin irq;
};

#endif /* BUTTON_H_ */
