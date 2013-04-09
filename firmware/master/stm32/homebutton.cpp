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
#include "usb/usbdevice.h"
bool btle_flag = false;
IRQ_HANDLER ISR_EXTI2()
{
    if (btle_flag) {
        nrf51822::btle_swd.readRequest(0xa5); //Read ID
        btle_flag = false;
    } else {
        nrf51822::btle_swd.readRequest(0xb1);   //Read SELECT
        btle_flag = true;
    }
    //uint32_t value = nrf51822::btle_swd.payloadIn;
    //const uint8_t response[] = { 255, value & 0xff, (value >> 8) & 0xff, (value >> 16) & 0xff, (value >> 24) & 0xff };
    //UsbDevice::write(response, sizeof response);
    BTN_HOME_GPIO.irqAcknowledge();
    HomeButton::update();
    Pause::taskWork.atomicMark(Pause::ButtonPress);
    Tasks::trigger(Tasks::Pause);
}
#endif

