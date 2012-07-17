#include "powermanager.h"
#include "board.h"
#include "usb/usbdevice.h"
#include "systime.h"
#include "tasks.h"

#include "macros.h"

PowerManager::State PowerManager::_state;
GPIOPin PowerManager::vbus = USB_VBUS_GPIO;
static SysTime::Ticks railTransitionDeadline;

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

    GPIOPin vcc3v3 = VCC33_ENABLE_GPIO;
    vcc3v3.setControl(GPIOPin::OUT_2MHZ);
    vcc3v3.setLow();

    // set initial state
    _state = PowerManager::Uninitialized;
    vbus.setControl(GPIOPin::IN_FLOAT);

    // and start listening for edges
    vbus.irqInit();
    vbus.irqSetFallingEdge();
    vbus.irqSetRisingEdge();
    vbus.irqEnable();
}

/*
 * Called from ISR context when an edge on vbus is detected.
 */
void PowerManager::onVBusEdge()
{
    railTransitionDeadline = SysTime::ticks() + SysTime::msTicks(10); //10ms debounce time
    Tasks::setPending(Tasks::PowerManager);
}

/*
 * Provides for a little settling time from the last edge
 * on vbus before initiating the actual rail transition
 */
void PowerManager::railTransition(void* p)
{
    if (SysTime::ticks() < railTransitionDeadline)
        return;

    Tasks::clearPending(Tasks::PowerManager);

    State s = static_cast<State>(vbus.isHigh());
    if (s != _state) {

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

        default:
            break;
        }
#endif

        _state = s;
    }
}

void PowerManager::shutdown()
{
    // release the power supply enable
    GPIOPin vcc20 = VCC20_ENABLE_GPIO;
    vcc20.setControl(GPIOPin::OUT_2MHZ);
    vcc20.setLow();
}

