#include "powermanager.h"
#include "board.h"

#include "macros.h"

PowerManager::State PowerManager::_state;
GPIOPin PowerManager::vbus = USB_VBUS_GPIO;

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

    // set initial state
    _state = PowerManager::Uninitialized;
    vbus.setControl(GPIOPin::IN_FLOAT);
    onVBusEdge();

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
    State s = static_cast<State>(vbus.isHigh());
    if (s != _state) {

#if (BOARD >= BOARD_TC_MASTER_REV2)
        /*
         * The order in which we sequence these enable lines is important,
         * especially to ensure that the flash doesn't see 3v before its
         * regulator has been enabled.
         * - on transition to usb power, enable flash then enable 3v3
         * - on transition to battery power, disable 3v3 then disable flash
         */

        GPIOPin vcc3v3 = VCC33_ENABLE_GPIO;

        switch (s) {
        case BatteryPwr:
            vcc3v3.setLow();
            break;

        case UsbPwr:
            vcc3v3.setHigh();
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

