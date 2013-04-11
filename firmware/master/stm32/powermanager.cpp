#include "powermanager.h"
#include "board.h"
#include "usb/usbdevice.h"
#include "systime.h"
#include "tasks.h"
#include "radio.h"
#include "batterylevel.h"

#include "macros.h"

PowerManager::State PowerManager::lastState;
GPIOPin PowerManager::vbus = USB_VBUS_GPIO;
SysTime::Ticks PowerManager::debounceDeadline;

void PowerManager::batteryPowerOn()
{
    /*
     * Keep battery power on. Must happen before the
     * user releases our home button, if we're on battery power,
     * so this runs during very early init.
     */
#if BOARD != BOARD_TEST_JIG

#ifdef HAS_SINGLE_RAIL
    GPIOPin powerEnable = VCC30_ENABLE_GPIO;
#else
    GPIOPin powerEnable = VCC20_ENABLE_GPIO;
#endif

    powerEnable.setControl(GPIOPin::OUT_2MHZ);
    powerEnable.setHigh();
#endif
}

void PowerManager::batteryPowerOff()
{
    // release the power supply enable

#if BOARD != BOARD_TEST_JIG

#ifdef HAS_SINGLE_RAIL
    GPIOPin powerEnable = VCC30_ENABLE_GPIO;
#else
    GPIOPin powerEnable = VCC20_ENABLE_GPIO;
#endif

    powerEnable.setControl(GPIOPin::OUT_2MHZ);
    powerEnable.setLow();
#endif
}

/*
 * Other initializations that can happen after global ctors have run.
 *
 * Important: the flash LDO must get enabled before the 3v3 line, or we
 * risk bashing the flash IC with too much voltage.
 */
void PowerManager::init()
{
#if BOARD != BOARD_TEST_JIG && !defined(HAS_SINGLE_RAIL)
    GPIOPin flashRegEnable = FLASH_REG_EN_GPIO;
    flashRegEnable.setControl(GPIOPin::OUT_2MHZ);
    flashRegEnable.setHigh();

    GPIOPin vcc3v3 = VCC33_ENABLE_GPIO;
    vcc3v3.setControl(GPIOPin::OUT_2MHZ);
#endif

    vbus.setControl(GPIOPin::IN_FLOAT);

    /*
     * Set initial state.
     *
     * Do this based on a poll of vbus, since our debounce process won't
     * start up until tasks are running, and we need to init things before that:
     * particularly the nRF, which has some specific power on timing requirements.
     */
    setState(state());
}

/*
 * Enable interrupts that monitor vbus, which in turn manage the USB device.
 * USB and GPIO interrupts must be enabled.
 *
 * It's helpful to separate this from init() so we can keep the USB device
 * disabled in the bootloader if an update is not required.
 */
void PowerManager::beginVbusMonitor()
{
    onVBusEdge();

    // and start listening for edges
    vbus.irqInit();
    vbus.irqSetFallingEdge();
    vbus.irqSetRisingEdge();
    vbus.irqEnable();
}

/*
 * Called from ISR context when an edge on vbus is detected.
 * We wait for DEBOUNCE_MILLIS and then apply the current state at that point.
 */
void PowerManager::onVBusEdge()
{
    debounceDeadline = SysTime::ticks() + SysTime::msTicks(DEBOUNCE_MILLIS);
    Tasks::trigger(Tasks::PowerManager);
}

/*
 * Provides for a little settling time from the last edge
 * on vbus before initiating the actual rail transition
 */
void PowerManager::vbusDebounce()
{
    if (SysTime::ticks() < debounceDeadline) {
        // Poll until our deadline is elapsed
        Tasks::trigger(Tasks::PowerManager);
        return;
    }

    State s = state();
    if (s != lastState) {
        setState(s);
#if !defined(BOOTLOADER) && (BOARD != BOARD_TEST_JIG)
        if (s == UsbPwr) {
            Radio::onTransitionToUsbPower();
        }
#endif
    }
}

void PowerManager::setState(State s)
{
#if (BOARD >= BOARD_TC_MASTER_REV2) && (!defined HAS_SINGLE_RAIL)
    GPIOPin vcc3v3 = VCC33_ENABLE_GPIO;

    switch (s) {
    case BatteryPwr:
        batteryPowerOn();
        UsbDevice::deinit();
        vcc3v3.setLow();
        break;

    case UsbPwr:
        vcc3v3.setHigh();
        UsbDevice::init();
        batteryPowerOff();
        break;
    }
#elif defined HAS_SINGLE_RAIL

    // unconditionally keep the power on.
    batteryPowerOn();

    switch(s) {
    case BatteryPwr:
        UsbDevice::deinit();
        break;

    case UsbPwr:
        UsbDevice::init();
        break;
    }
#endif

    lastState = s;
}

void PowerManager::shutdownIfVBattIsCritical(unsigned vbatt, unsigned limit)
{
#ifndef USE_ADC_BATT_MEAS
    /*
     * If we're measuring via RC samples can have non-trivial jitter,
     * so we assume that any sample we take is skewed worst case by
     * our MAX_JITTER in the positive direction. Correct for this, and see if
     * we're still above the required thresh to stay alive.
     */
    vbatt -= BatteryLevel::MAX_JITTER;
#endif

    if (vbatt < limit) {

        // shut ourselves down, and hope our batteries get replaced soon.
        batteryPowerOff();

        /*
         * wait to for power to drain. if somebody keeps their finger
         * on the homebutton, we may be here a little while, so don't
         * get zapped by the watchdog on our way out
         */
        for (;;) {
            Atomic::Barrier();
            Tasks::resetWatchdog();
        }
    }
}
