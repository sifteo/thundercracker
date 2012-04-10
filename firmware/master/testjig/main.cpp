/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "usart.h"
#include "board.h"
#include "flash.h"
#include "systime.h"
#include "radio.h"
#include "tasks.h"
#include "audiomixer.h"
#include "audiooutdevice.h"
#include "usb/usbdevice.h"
#include "testjig.h"

/*
 * Test Jig application specific entry point.
 * Lower level init happens in setup.cpp.
 */
int main()
{
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

//    NVIC.irqEnable(IVT.TIM3);                   // sample rate timer
//    NVIC.irqPrioritize(IVT.TIM3, 0x60);         //  Higher prio than radio

    /*
     * High-level hardware initialization
     */

    SysTime::init();
    Radio::open();
    Tasks::init();
    FlashBlock::init();

    AudioMixer::instance.init();
    AudioOutDevice::init(AudioOutDevice::kHz16000, &AudioMixer::instance);
    AudioOutDevice::start();

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
