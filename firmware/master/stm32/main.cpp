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
#include "cubeconnector.h"
#include "neighbor_tx.h"
#include "led.h"
#include "batterylevel.h"
#include "adc.h"
#include "nrf51822.h"

/*
 * Application specific entry point.
 * All low level init is done in setup.cpp.
 */
int main()
{
    /*
     * Nested Vectored Interrupt Controller setup.
     *
     * This won't actually enable any peripheral interrupts yet, since
     * those need to be unmasked by the peripheral's driver code.
     *
     * If we've gotten bootloaded, relocate the vector table to account
     * for offset at which we're placed into MCU flash.
     */

#ifdef BOOTLOADABLE
    NVIC.setVectorTable(NVIC.VectorTableFlash, Bootloader::SIZE);
#endif

    NVIC.irqEnable(IVT.RF_EXTI_VEC);                // Radio interrupt
    NVIC.irqPrioritize(IVT.RF_EXTI_VEC, 0x80);      //  Reduced priority

    NVIC.irqEnable(IVT.RF_DMA_CHAN_RX);              // Radio SPI, DMA2 channels 1 & 2
    NVIC.irqPrioritize(IVT.RF_DMA_CHAN_RX, 0x75);
    NVIC.irqEnable(IVT.RF_DMA_CHAN_TX);
    NVIC.irqPrioritize(IVT.RF_DMA_CHAN_TX, 0x75);

    NVIC.irqEnable(IVT.FLASH_DMA_CHAN_RX);          // Flash SPI DMA channels
    NVIC.irqPrioritize(IVT.FLASH_DMA_CHAN_RX, 0x74);//  higher than radio, since flash is higher bandwidth
    NVIC.irqEnable(IVT.FLASH_DMA_CHAN_TX);
    NVIC.irqPrioritize(IVT.FLASH_DMA_CHAN_TX, 0x74);

    NVIC.irqEnable(IVT.UsbOtg_FS);
    NVIC.irqPrioritize(IVT.UsbOtg_FS, 0x70);        //  A little higher than radio

    NVIC.irqEnable(IVT.BTN_HOME_EXTI_VEC);          //  home button

    NVIC.irqEnable(IVT.BTLE_SWD_TIM_INT);           // swd timer
    NVIC.irqPrioritize(IVT.BTLE_SWD_TIM_INT, 0x80); //

#ifdef USE_AUDIO_DAC
    NVIC.irqEnable(IVT.AUDIO_DAC_DMA_IRQ);          // DAC DMA channel
    NVIC.irqPrioritize(IVT.AUDIO_DAC_DMA_IRQ, 0x50);
#else
    NVIC.irqEnable(IVT.AUDIO_SAMPLE_TIM);           // sample rate timer
    NVIC.irqPrioritize(IVT.AUDIO_SAMPLE_TIM, 0x50); //  pretty high priority! (would cause audio jitter)
#endif

    NVIC.irqEnable(IVT.LED_SEQUENCER_TIM);          // LED sequencer timer
    NVIC.irqPrioritize(IVT.LED_SEQUENCER_TIM, 0x85);

    NVIC.irqEnable(IVT.USART3);                     // factory test uart
    NVIC.irqPrioritize(IVT.USART3, 0x52);           //  high enough to avoid overruns

#ifndef USE_ADC_FADER_MEAS
    NVIC.irqEnable(IVT.VOLUME_TIM);                 // volume timer
    NVIC.irqPrioritize(IVT.VOLUME_TIM, 0x55);       //  just below sample rate timer
#endif

    NVIC.irqEnable(IVT.PROFILER_TIM);               // sample profiler timer
    NVIC.irqPrioritize(IVT.PROFILER_TIM, 0x0);      //  highest possible priority

    NVIC.irqEnable(IVT.NBR_TX_TIM);                 // Neighbor transmit
    NVIC.irqPrioritize(IVT.NBR_TX_TIM, 0x60);       //  just below volume timer

#if defined USE_ADC_BATT_MEAS ||  defined USE_ADC_FADER_MEAS
    NVIC.irqEnable(IVT.ADC1_2);                     // adc sample
    NVIC.irqPrioritize(IVT.ADC1_2,0x80);            // low priority. only used for battery/fader measurement
#endif

    /*
     * For SVM to operate properly, SVC needs to have a very low priority
     * (we'll be inside it most of the time) and any fault handlers which have
     * meaning in userspace need to be higher priority than it.
     *
     * We disable the local fault handlers, (should be disabled by default anyway)
     * so all faults will get routed through HardFault and handled by SvmCpu.
     */

    NVIC.sysHandlerPrioritize(IVT.SVCall, 0x96);
    NVIC.sysHandlerPrioritize(IVT.HardFault, 0x00);
    NVIC.sysHandlerControl = 0;

    /*
     * High-level hardware initialization
     *
     * Avoid reinitializing periphs that the bootloader has already init'd.
     */
#ifndef BOOTLOADABLE
    SysTime::init();
    PowerManager::init();
    Crc32::init();
#endif

    // This is the earliest point at which it's safe to use Usart::Dbg.
    Usart::Dbg.init(UART_RX_GPIO, UART_TX_GPIO, 115200);

#ifdef REV2_GDB_REWORK
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

    #if (defined USE_ADC_BATT_MEAS) || (defined USE_ADC_FADER_MEAS)
    Adc::Adc1.init();
    Adc::Adc1.enableInterrupt();
    #endif

    LED::init();
    Tasks::init();
    FlashStack::init();
    HomeButton::init();
    NeighborTX::init();
    nrf51822::test();

    /*
     * NOTE: NeighborTX & BatteryLevel share a timer - Battery level expects
     *      it to have already been init'd.
     *
     *      Also, CubeConnector is currently the only system to initiate neighbor
     *      transmissions, so wait until we do our battery check before init'ing
     *      him to avoid any conflicts.
     */

    BatteryLevel::init();

#ifdef USE_ADC_BATT_MEAS
    // Delay required to charge up the internal reference cap.
    // It takes approximately 250 ms with R18 100k pullup.
    while (SysTime::ticks() < SysTime::msTicks(300)) {
        ;
    }
#endif

#ifndef HAS_SINGLE_RAIL
    if (PowerManager::state() == PowerManager::BatteryPwr) {
#endif

        /*
         * Ensure we have enough juise to make it worth starting up!
         *
         * Kick off our first sample and wait for it to complete.
         * Once our first vsys and vbatt samples have been taken, the
         * PowerManager will shut us down if we're at a critical level.
         */

        BatteryLevel::beginCapture();

        // we unforuntately don't have anything better to do while we wait, since
        // we can't proceed until we know this is OK. Our startup process is
        // ultimately bottlenecked by the radio's power on delay anyway, and
        // this time is taken into account for that, so we're still OK.
        while (BatteryLevel::vsys() == BatteryLevel::UNINITIALIZED ||
               BatteryLevel::raw() == BatteryLevel::UNINITIALIZED)
            ;

#ifndef HAS_SINGLE_RAIL
    }
#endif

    // wait until after we know we're going to continue starting up before
    // showing signs of life :)
    UART(("Firmware " TOSTRING(SDK_VERSION) "\r\n"));

    CubeConnector::init();

    Volume::init();
    AudioOutDevice::init();
    AudioOutDevice::start();

    PowerManager::beginVbusMonitor();
    SampleProfiler::init();

    // Includes radio power-on delay. Initialize this last.
    Radio::init();

    /*
     * Start the game runtime, and execute the Launcher app.
     */

    SvmLoader::runLauncher();
}
