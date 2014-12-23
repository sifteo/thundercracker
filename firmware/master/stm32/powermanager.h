/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

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

    static void standby();
    static void stop();

    static void shutdownIfVBattIsCritical(unsigned vbatt, unsigned vsys);

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
