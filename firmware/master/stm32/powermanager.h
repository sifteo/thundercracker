#ifndef POWERMANAGER_H
#define POWERMANAGER_H

#include "gpio.h"
#include "systime.h"

class PowerManager
{
public:
    enum State {
        BatteryPwr      = 0,
        UsbPwr          = 1
    };

    static void init();
    static void beginVbusMonitor();

    static ALWAYS_INLINE State state() {
        return static_cast<State>(vbus.isHigh());
    }

    static void vbusDebounce();

    static void batteryPowerOn();
    static void batteryPowerOff();

    // only exposed for use via exti
    static GPIOPin vbus;
    static void onVBusEdge();

private:
    static const unsigned DEBOUNCE_MILLIS = 10;
    static SysTime::Ticks debounceDeadline;
    static State lastState;

    static void setState(State s);
};

#endif // POWERMANAGER_H
