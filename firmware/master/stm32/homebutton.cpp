/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "homebutton.h"
#include "gpio.h"
#include "board.h"
#include "tasks.h"

static GPIOPin homeButton = BTN_HOME_GPIO;

namespace HomeButton {

void init()
{
    homeButton.setControl(GPIOPin::IN_FLOAT);
    homeButton.irqInit();
    homeButton.irqSetRisingEdge();
    homeButton.irqEnable();
}

bool isPressed()
{
    return homeButton.isHigh();
}

} // namespace HomeButton


#if (BOARD >= BOARD_TC_MASTER_REV2)
IRQ_HANDLER ISR_EXTI2()
{
    homeButton.irqAcknowledge();
    Tasks::trigger(Tasks::HomeButton);
}
#endif

