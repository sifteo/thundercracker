/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "usart.h"
#include "board.h"
#include "systime.h"
#include "radio.h"
#include "tasks.h"
#include "usb/usbdevice.h"
#include "testjig.h"
#include "gpio.h"
#include "bootloader.h"

/*
 * Test Jig application specific entry point.
 * Lower level init happens in setup.cpp.
 */
int main()
{
  
    #ifdef BOOTLOADABLE
        NVIC.setVectorTable(NVIC.VectorTableFlash, Bootloader::SIZE);
    #endif
    
    /*
     * Nested Vectored Interrupt Controller setup.
     *
     * This won't actually enable any peripheral interrupts yet, since
     * those need to be unmasked by the peripheral's driver code.
     */

    NVIC.irqEnable(IVT.EXTI9_5);                // Radio interrupt
    NVIC.irqPrioritize(IVT.EXTI9_5, 0x80);      //  Reduced priority

    NVIC.irqEnable(IVT.UsbOtg_FS);
    NVIC.irqPrioritize(IVT.UsbOtg_FS, 0x90);    //  Lower prio than radio

    //Set an LED high so we know we're up and running.
    GPIOPin power = LED_GREEN2_GPIO;
    power.setControl(GPIOPin::OUT_2MHZ);
    power.setHigh();
    
    /*
     * High-level hardware initialization
     */

    SysTime::init();
    Radio::init();
    Tasks::init();

    UsbDevice::init();

    /*
     * Once the TestJig is initialized, test commands from the host will arrive
     * over USB and get processed within Tasks::work().
     */
    TestJig::init();

    for (;;) {
        Tasks::work();
    }
}
