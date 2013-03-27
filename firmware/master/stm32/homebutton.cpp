/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "homebutton.h"
#include "gpio.h"
#include "board.h"
#include "tasks.h"
#include "pause.h"

namespace HomeButton {

void init()
{
    GPIOPin homeButton = BTN_HOME_GPIO;
    homeButton.setControl(GPIOPin::IN_FLOAT);
    homeButton.irqInit();
    homeButton.irqSetRisingEdge();
    homeButton.irqSetFallingEdge();
    homeButton.irqEnable();
}

bool isPressed()
{
    return BTN_HOME_GPIO.isHigh();
}

} // namespace HomeButton


#if (BOARD >= BOARD_TC_MASTER_REV2)
#include "nrf51822.h"
IRQ_HANDLER ISR_EXTI2()
{
    while(!nrf51822::btle_swd.readRequest(0xa5)); //0b10100101)
    BTN_HOME_GPIO.irqAcknowledge();
    HomeButton::update();
    Pause::taskWork.atomicMark(Pause::ButtonPress);
    Tasks::trigger(Tasks::Pause);
}
#endif

