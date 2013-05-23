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
    
    GPIOPin dip3 = DIP_SWITCH3_GPIO;
    dip3.setControl(GPIOPin::IN_PULL);
    dip3.pullup();
    /*
     * High-level hardware initialization
     */

    SysTime::init();

    // conditionally fire up USB peripheral as long as the dip
    // switch is off! enables the use of a testjig as a dummy power supply
    if(dip3.isHigh()) {
        UsbDevice::init();   // fires up USB
    }

    Tasks::init();

    // This is the earliest point at which it's safe to use Usart::Dbg.
    Usart::Dbg.init(UART_RX_GPIO, UART_TX_GPIO, 115200);

    UART("Sifteo Testjig\r\nVersion: " TOSTRING(SDK_VERSION) "\r\n");

    /*
     * Once the TestJig is initialized, test commands from the host will arrive
     * over USB and get processed within Tasks::work().
     */
    TestJig::init();

    for (;;) {
        Tasks::work();
    }
}
