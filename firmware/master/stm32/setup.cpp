/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Low level hardware setup for the STM32 board.
 */

#include <sifteo.h>
#include "radio.h"
#include "usb.h"
#include "flashlayer.h"
#include "runtime.h"
#include "board.h"
#include "vectors.h"
#include "systime.h"
#include "gpio.h"
#include "macronixmx25.h"
#include "tasks.h"
#include "audiomixer.h"
#include "audiooutdevice.h"
#include "usart.h"
#include "button.h"

/* One function in the init_array segment */
typedef void (*initFunc_t)(void);

/* Addresses defined by our linker script */
extern "C" unsigned __bss_start;
extern "C" unsigned __bss_end;
extern "C" unsigned __data_start;
extern "C" unsigned __data_end;
extern "C" unsigned __data_src;
extern "C" initFunc_t __init_array_start;
extern "C" initFunc_t __init_array_end;

extern "C" void _start()
{
    /*
     * Set up clocks:
     *   - 8 MHz HSE (xtal) osc
     *   - PLL x9 => 72 MHz
     *   - SYSCLK at 72 MHz
     *   - HCLK at 72 MHz
     *   - APB1 at 18 MHz (/4)
     *       - SPI2 at 9 MHz
     *   - APB2 at 36 MHz (/2)
     *       - GPIOs
     *   - USB clock at 48 MHz (PLL /1)
     *
     * Other things that depend on our clock setup:
     *
     *   - SPI configuration. Keep nRF SPI as close to 10 MHz as we
     *     can without going over.
     *
     *   - SysTick frequency, in systime.cpp. The Cortex-M3's
     *     system clock is 1/8th the AHB clock.
     */

    // system runs from HSI on reset - make sure this is on and stable
    // before we switch away from it
    RCC.CR |= (1 << 0); // HSION
    while (!(RCC.CR & (1 << 1))); // wait for HSI ready

    RCC.CR &=  (0x1F << 3)  |   // HSITRIM reset value
               (1 << 0);        // HSION
    RCC.CFGR = 0;       // reset
    // Wait until HSI is the source.
    while ((RCC.CFGR & (3 << 2)) != 0x0);

    // fire up HSE
    RCC.CR |= (1 << 16); // HSEON
    while (!(RCC.CR & (1 << 17))); // wait for HSE to be stable

    // fire up the PLL
    RCC.CFGR |= (7 << 18) |         // PLLMUL (x9)
                (0 << 17) |         // PLL XTPRE - no divider
                (1 << 16);          // PLLSRC - HSE
    RCC.CR   |= (1 << 24);          // turn PLL on
    while (!(RCC.CR & (1 << 25)));  // wait for PLL to be ready

    // configure all the other buses
    RCC.CFGR =  (0 << 24)       |   // MCO - mcu clock output
                (0 << 22)       |   // USBPRE - divide by 3
                (7 << 18)       |   // PLLMUL - x9
                (0 << 17)       |   // PLLXTPRE - no divider
                (1 << 16)       |   // PLLSRC - HSE
                (4 << 11)       |   // PPRE2 - APB2 prescaler, divide by 2
                (5 << 8)        |   // PPRE1 - APB1 prescaler, divide by 4
                (0 << 4);           // HPRE - AHB prescaler, no divisor

    FLASH.ACR = (1 << 4) |  // prefetch buffer enable
                (1 << 1);   // two wait states since we're @ 72MHz

    // switch to PLL as system clock
    RCC.CFGR |= (2 << 0);
    while ((RCC.CFGR & (3 << 2)) != (2 << 2));   // wait till we're running from PLL

    // reset all peripherals
    RCC.APB1RSTR = 0xFFFFFFFF;
    RCC.APB1RSTR = 0;
    RCC.APB2RSTR = 0xFFFFFFFF;
    RCC.APB2RSTR = 0;

    // Enable peripheral clocks
    RCC.APB1ENR = 0x00004000;    // SPI2
    RCC.APB2ENR = 0x0000003d;    // GPIO/AFIO
    RCC.AHBENR  = 0x00001040;    // USB OTG, CRC

#if 0
    // debug the clock output - MCO
    GPIOPin mco(&GPIOA, 8); // PA8 on the keil MCBSTM32E board - change as appropriate
    mco.setControl(GPIOPin::OUT_ALT_50MHZ);
#endif

    {
        GPIOPin vcc20 = VCC20_ENABLE_GPIO;
        GPIOPin vcc33 = VCC33_ENABLE_GPIO;

        vcc20.setControl(GPIOPin::OUT_10MHZ);
        vcc33.setControl(GPIOPin::OUT_10MHZ);

        vcc20.setHigh();
        vcc33.setHigh();
    }

    /*
     * Initialize data segments (In parallel with oscillator startup)
     */

    memset(&__bss_start, 0, (uintptr_t)&__bss_end - (uintptr_t)&__bss_start);
    memcpy(&__data_start, &__data_src,
           (uintptr_t)&__data_end - (uintptr_t)&__data_start);

    /*
     * Run C++ global constructors.
     *
     * Best-practice for these is to keep them limited to only
     * initializing data: We shouldn't be talking to hardware, or
     * doing anything long-running here. Think of it as a way to
     * programmatically unpack data from flash to RAM, just like we
     * did above with the .data segment.
     */
    
    for (initFunc_t *p = &__init_array_start; p != &__init_array_end; p++)
        p[0]();


    AFIO.MAPR |= (0x4 << 24);       // disable JTAG so we can talk to flash
    MacronixMX25::instance.init();

    /*
     * Nested Vectored Interrupt Controller setup.
     *
     * This won't actually enable any peripheral interrupts yet, since
     * those need to be unmasked by the peripheral's driver code.
     */

    NVIC.irqEnable(IVT.EXTI9_5);                // Radio interrupt
    NVIC.irqPrioritize(IVT.EXTI9_5, 0x80);      //   Reduced priority

    NVIC.irqEnable(IVT.UsbOtg_FS);
    NVIC.irqPrioritize(IVT.UsbOtg_FS, 0x90);

    NVIC.irqEnable(IVT.EXTI0);                  // home button

    /*
     * High-level hardware initialization
     */

    Usart::Dbg.init(UART_RX_GPIO, UART_TX_GPIO, 115200);

    SysTime::init();
    Radio::open();
    Tasks::init();
    FlashLayer::init();
    Button::init();

	//eliao, commenting out temporarily to get under stm limit
    /*AudioMixer::instance.init();
    AudioOutDevice::init(AudioOutDevice::kHz16000, &AudioMixer::instance);
    AudioOutDevice::start();*/

#if 0
    // ALERT! ST's usb library appears to overwrite registers related to
    // SysTick and as such, cannot be used while you want to talk to cubes
    // over the radio. It's fine for loading data over USB, though.
    Usb::init();
    // super hack: just wait around for data to be loaded. revel in the crappiness
    // as you disable this block and reflash your board to re-enable SysTick.
    for (;;) {
        Sifteo::System::yield();
    }
#endif

    /*
     * Launch our game runtime!
     */

    Runtime::run();
}

extern "C" void *_sbrk(intptr_t increment)
{
#if 1
    // speex needs to alloc some memory on init...this allows it to but
    // doesn't allow alloc'd mem to be reclaimed. just a hack until we decide what to do.
    // NOTE - this also requires passing -fno-threasafe-statics in your CPPFLAGS
    // since GCC will normally create guards around static data
    static uintptr_t __heap = (uintptr_t)&__bss_end + 4;

    void *p = (void*)__heap;
    __heap += increment;
    return p;
#endif
    /*
     * We intentionally don't want to support dynamic allocation yet.
     * If anyone tries a malloc(), we'll just trap here infinitely.
     *
     * XXX: Either implement this for reals (after figuring out what
     *      limitations to put in place) or direct it at a proper fault
     *      handler once we have those.
     */

    while (1);
    return NULL;
}
