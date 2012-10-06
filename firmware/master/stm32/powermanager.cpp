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
    GPIOPin vcc20 = VCC20_ENABLE_GPIO;
    vcc20.setControl(GPIOPin::OUT_2MHZ);
    vcc20.setHigh();
}

void PowerManager::batteryPowerOff()
{
    // release the power supply enable
    GPIOPin vcc20 = VCC20_ENABLE_GPIO;
    vcc20.setControl(GPIOPin::OUT_2MHZ);
    vcc20.setLow();
}

/*
 * Other initializations that can happen after global ctors have run.
 *
 * Important: the flash LDO must get enabled before the 3v3 line, or we
 * risk bashing the flash IC with too much voltage.
 */
void PowerManager::init()
{
    GPIOPin flashRegEnable = FLASH_REG_EN_GPIO;
    flashRegEnable.setControl(GPIOPin::OUT_2MHZ);
    flashRegEnable.setHigh();

    vbus.setControl(GPIOPin::IN_FLOAT);

    GPIOPin vcc3v3 = VCC33_ENABLE_GPIO;
    vcc3v3.setControl(GPIOPin::OUT_2MHZ);

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
#if !defined(BOOTLOADER)
        if (s == UsbPwr) {
            Radio::onTransitionToUsbPower();
        }
#endif
    }
}

void PowerManager::setState(State s)
{
#if (BOARD >= BOARD_TC_MASTER_REV2)
    GPIOPin vcc3v3 = VCC33_ENABLE_GPIO;

    switch (s) {
    case BatteryPwr:
        UsbDevice::deinit();
        vcc3v3.setLow();
        break;

    case UsbPwr:
        vcc3v3.setHigh();
        UsbDevice::init();
        break;
    }
#endif

    lastState = s;
}

void PowerManager::shutdownIfVBattIsCritical(unsigned vbatt, unsigned vsys)
{
    /*
     * To be a bit conservative, we assume that any sample we take is skewed by
     * our MAX_JITTER in the positive direction. Correct for this, and see if
     * we're still above the required thresh to continue powering on.
     *
     * If not, shut ourselves down, and hope our batteries get replaced soon.
     */

    if (vbatt < vsys - BatteryLevel::MAX_JITTER) {

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
