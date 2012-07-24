#include "powermanager.h"
#include "board.h"
#include "usb/usbdevice.h"
#include "systime.h"
#include "tasks.h"

#include "macros.h"

PowerManager::State PowerManager::lastState;
GPIOPin PowerManager::vbus = USB_VBUS_GPIO;
SysTime::Ticks PowerManager::debounceDeadline;

/*
 * Early initialization that can (and must) run before global ctors have been run.
 */
void PowerManager::earlyInit()
{
    GPIOPin vcc20 = VCC20_ENABLE_GPIO;
    vcc20.setControl(GPIOPin::OUT_2MHZ);
    vcc20.setHigh();
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
    if (s != lastState)
        setState(s);
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

void PowerManager::shutdown()
{
    // release the power supply enable
    GPIOPin vcc20 = VCC20_ENABLE_GPIO;
    vcc20.setControl(GPIOPin::OUT_2MHZ);
    vcc20.setLow();
}
