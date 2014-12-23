/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
