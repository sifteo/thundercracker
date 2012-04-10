/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "usart.h"
#include "flash.h"
#include "hardware.h"
#include "board.h"
#include "gpio.h"
#include "systime.h"
#include "radio.h"
#include "tasks.h"
#include "flashlayer.h"
#include "audiomixer.h"
#include "audiooutdevice.h"
#include "usb/usbdevice.h"
#include "button.h"
#include "svmloader.h"

/*
 * Application specific entry point.
 * All low level init is done in setup.cpp.
 */
int main()
{
    // This is the earliest point at which it's safe to use Usart::Dbg.
    Usart::Dbg.init(UART_RX_GPIO, UART_TX_GPIO, 115200);

#ifndef DEBUG
    Flash::init();
#else
    DBGMCU_CR |= (1 << 30) |        // TIM14 stopped when core is halted
                 (1 << 29) |        // TIM13 ""
                 (1 << 28) |        // TIM12 ""
                 (1 << 27) |        // TIM11 ""
                 (1 << 26) |        // TIM10 ""
                 (1 << 25) |        // TIM9 ""
                 (1 << 20) |        // TIM8 ""
                 (1 << 19) |        // TIM7 ""
                 (1 << 18) |        // TIM6 ""
                 (1 << 17) |        // TIM5 ""
                 (1 << 13) |        // TIM4 ""
                 (1 << 12) |        // TIM3 ""
                 (1 << 11) |        // TIM2 ""
                 (1 << 10);         // TIM1 ""
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

    NVIC.irqEnable(IVT.BTN_HOME_EXTI_VEC);      //  home button

    NVIC.irqEnable(IVT.TIM4);                   // sample rate timer
    NVIC.irqPrioritize(IVT.TIM4, 0x60);         //  Higher prio than radio

    NVIC.sysHandlerPrioritize(IVT.SVCall, 0x96);

    /*
     * High-level hardware initialization
     */

    SysTime::init();
    Radio::open();
    Tasks::init();
    FlashBlock::init();
    Button::init();

    AudioMixer::instance.init();
    AudioOutDevice::init(AudioOutDevice::kHz16000, &AudioMixer::instance);
    AudioOutDevice::start();

    UsbDevice::init();

    /*
     * Temporary until we have a proper context to install new games in.
     * If button is held on startup, wait for asset installation.
     *
     * Kind of crappy, but just power cycle to start again and run the game.
     */
    if (Button::isPressed()) {

        // indicate we're waiting
        GPIOPin green = LED_GREEN_GPIO;
        green.setControl(GPIOPin::OUT_10MHZ);
        green.setLow();

        for (;;)
            Tasks::work();
    }

    /*
     * Launch our game runtime!
     */

    SvmLoader::run(111);

    // for now, in the event that we don't have a valid game installed at address 0,
    // SvmLoader::run() should return (assuming it fails to parse the non-existent
    // ELF binary, and we'll just sit here so we can at least install things over USB, etc
    for (;;) {
        Tasks::work();
    }
}
