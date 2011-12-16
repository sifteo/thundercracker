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

#include <sifteo/abi.h>
#include "radio.h"
#include "runtime.h"
#include "hardware.h"
#include "vectors.h"
#include "systime.h"

#include "usbd_usr.h"
#include "usbd_desc.h"
#include "usbd_cdc_core.h"
#include "usbd_core.h"

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
    RCC_SIFTEO.CR |= (1 << 0); // HSION
    while (!(RCC_SIFTEO.CR & (1 << 1))); // wait for HSI ready

    RCC_SIFTEO.CR &=  (0x1F << 3)  |   // HSITRIM reset value
               (1 << 0);        // HSION
    RCC_SIFTEO.CFGR = 0;       // reset
    // Wait until HSI is the source.
    while ((RCC_SIFTEO.CFGR & (3 << 2)) != 0x0);

    // fire up HSE
    RCC_SIFTEO.CR |= (1 << 16); // HSEON
    while (!(RCC_SIFTEO.CR & (1 << 17))); // wait for HSE to be stable

    // PLL2 configuration: PLL2CLK = (HSE / 5) * 8 = 40 MHz
    // PREDIV1 configuration: PREDIV1CLK = PLL2 / 5 = 8 MHz
    RCC_SIFTEO.CFGR2 &= ~0x10FFF;
    RCC_SIFTEO.CFGR2 |= 0x10644;
    RCC_SIFTEO.CR |= (1 << 26);             // turn PLL2 on
    while (!(RCC_SIFTEO.CR & (1 << 27)));   // wait for it to be ready

    // fire up the PLL
    RCC_SIFTEO.CFGR |= (7 << 18) |         // PLLMUL (x9)
                (0 << 17) |         // PLL XTPRE - no divider
                (1 << 16);          // PLLSRC - HSE
    RCC_SIFTEO.CR   |= (1 << 24);          // turn PLL on
    while (!(RCC_SIFTEO.CR & (1 << 25)));  // wait for PLL to be ready

    // configure all the other buses
    RCC_SIFTEO.CFGR =  (0 << 24)       |   // MCO - mcu clock output
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
    RCC_SIFTEO.CFGR |= (2 << 0);
    while ((RCC_SIFTEO.CFGR & (3 << 2)) != (2 << 2));   // wait till we're running from PLL

    // reset all peripherals
    RCC_SIFTEO.APB1RSTR = 0xFFFFFFFF;
    RCC_SIFTEO.APB1RSTR = 0;
    RCC_SIFTEO.APB2RSTR = 0xFFFFFFFF;
    RCC_SIFTEO.APB2RSTR = 0;

    // Enable peripheral clocks
    RCC_SIFTEO.APB1ENR = 0x00004000;    // SPI2
    RCC_SIFTEO.APB2ENR = 0x0000003d;    // GPIO/AFIO
    RCC_SIFTEO.AHBENR  = 0x00001000;    // USB OTG

#if 0
    // debug the clock output - MCO
    GPIOPin mco(&GPIOA, 8); // PA8 on the keil MCBSTM32E board - change as appropriate
    mco.setControl(GPIOPin::OUT_ALT_50MHZ);
#endif

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

    USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb, &USR_cb);

    /*
     * Nested Vectored Interrupt Controller setup.
     *
     * This won't actually enable any peripheral interrupts yet, since
     * those need to be unmasked by the peripheral's driver code.
     */

    NVIC_SIFTEO.irqEnable(IVT.EXTI15_10);              // Radio interrupt
    NVIC_SIFTEO.irqPrioritize(IVT.EXTI15_10, 0x80);    //   Reduced priority

    /*
     * High-level hardware initialization
     */

    SysTime::init();
    Radio::open();

    /*
     * Launch our game runtime!
     */

    Runtime::run();
}

extern "C" void *_sbrk(intptr_t increment)
{
#if 0
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
