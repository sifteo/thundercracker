/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "macros.h"
#include "usart.h"
#include "flash_stack.h"
#include "hardware.h"
#include "board.h"
#include "gpio.h"
#include "systime.h"
#include "radio.h"
#include "tasks.h"
#include "audiomixer.h"
#include "audiooutdevice.h"
#include "volume.h"
#include "usb/usbdevice.h"
#include "homebutton.h"
#include "svmloader.h"
#include "powermanager.h"
#include "crc.h"
#include "sampleprofiler.h"
#include "bootloader.h"

/*
 * Application specific entry point.
 * All low level init is done in setup.cpp.
 */
int main()
{
    /*
     * If we've gotten bootloaded, relocate the vector table to account
     * for offset at which we're placed into MCU flash.
     *
     * Avoid reinitializing periphs that the bootloader has already init'd:
     * - PowerManager
     * - SysTime
     * - Crc
     */
#ifdef BOOTLOADABLE
    NVIC.setVectorTable(NVIC.VectorTableFlash, Bootloader::SIZE);
#else
    PowerManager::init();
    SysTime::init();
    Crc32::init();
#endif

    /*
     * Nested Vectored Interrupt Controller setup.
     *
     * This won't actually enable any peripheral interrupts yet, since
     * those need to be unmasked by the peripheral's driver code.
     */

    NVIC.irqEnable(IVT.EXTI9_5);                    // Radio interrupt
    NVIC.irqPrioritize(IVT.EXTI9_5, 0x80);          //  Reduced priority

    NVIC.irqEnable(IVT.DMA2_Channel1);              // Radio SPI DMA2 channels 1 & 2
    NVIC.irqPrioritize(IVT.DMA1_Channel1, 0x75);    //  higher than radio
    NVIC.irqEnable(IVT.DMA2_Channel2);
    NVIC.irqPrioritize(IVT.DMA1_Channel2, 0x75);

    NVIC.irqEnable(IVT.DMA1_Channel2);              // Flash SPI DMA1 channels 2 & 3
    NVIC.irqPrioritize(IVT.DMA1_Channel2, 0x75);    //  higher than radio
    NVIC.irqEnable(IVT.DMA1_Channel3);
    NVIC.irqPrioritize(IVT.DMA1_Channel3, 0x75);

    NVIC.irqEnable(IVT.UsbOtg_FS);
    NVIC.irqPrioritize(IVT.UsbOtg_FS, 0x70);        //  Lower prio than radio

    NVIC.irqEnable(IVT.BTN_HOME_EXTI_VEC);          //  home button

    NVIC.irqEnable(IVT.TIM4);                       // sample rate timer
    NVIC.irqPrioritize(IVT.TIM4, 0x50);             //  pretty high priority! (would cause audio jitter)

    NVIC.irqEnable(IVT.USART3);                     // factory test uart
    NVIC.irqPrioritize(IVT.USART3, 0x99);           //  loooooowest prio

    NVIC.sysHandlerPrioritize(IVT.SVCall, 0x96);

    NVIC.irqEnable(IVT.VOLUME_TIM);                 // volume timer
    NVIC.irqPrioritize(IVT.VOLUME_TIM, 0x60);       //  just below sample rate timer

    NVIC.irqEnable(IVT.PROFILER_TIM);               // sample profiler timer
    NVIC.irqPrioritize(IVT.PROFILER_TIM, 0x0);      //  highest possible priority

    /*
     * High-level hardware initialization
     */

    // This is the earliest point at which it's safe to use Usart::Dbg.
    Usart::Dbg.init(UART_RX_GPIO, UART_TX_GPIO, 115200);
    UART(("Firmware " TOSTRING(SDK_VERSION) "\r\n"));

#ifdef DEBUG
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
     * NOTE: the radio has 2 100ms delays on a power on reset: one before
     * we can talk to it at all, and one before we can start transmitting.
     *
     * TODO: make the delays async, such that runtime init can progress
     * in parallel.
     *
     * For now, allow other initializations to run while we wait for the 2nd delay.
     */
    while (SysTime::ticks() < SysTime::msTicks(110));
    Radio::init();

    Tasks::init();
    FlashStack::init();
    HomeButton::init();

    Volume::init();
    AudioOutDevice::init(&AudioMixer::instance);
    AudioOutDevice::start();

    PowerManager::onVBusEdge();
    SampleProfiler::init();

    /*
     * Ensure we've been powered up long enough before beginning radio
     * transmissions. This may change as we integrate some new code that
     * tracks whether we're actively transmitting or not, since there's
     * a lot of overlap between this issue and some much needed
     * radio throttling / power management.
     */

    while (SysTime::ticks() < SysTime::msTicks(210));
    Radio::begin();

    /*
     * Start the game runtime, and execute the Launcher app.
     */

    SvmLoader::runLauncher();
}
