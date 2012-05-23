/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#include "gpio.h"

namespace Button
{
    void init();
    void onChange();
    bool isPressed();

} // namespace Button

#endif /* BUTTON_H_ */
