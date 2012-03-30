/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#include "gpio.h"

class Button
{
public:
    static void init();
    static void isr();

    static inline bool isPressed() {
        return homeButton.isHigh();
    }

private:
    static GPIOPin homeButton;
};

#endif /* BUTTON_H_ */
