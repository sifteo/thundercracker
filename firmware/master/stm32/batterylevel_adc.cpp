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

#include "batterylevel.h"
#include "board.h"
#include "gpio.h"
#include "adc.h"
#include "powermanager.h"
#include "systime.h"
#include "macros.h"

#include <sifteo/abi.h>

#ifdef USE_ADC_BATT_MEAS

/*
 * BatteryLevel measurement via ADC.
 * Used as of rev3, since we're now running at a voltage
 * that supports the ADC hardware.
 *
 * Earlier hardware revs make use of batterylevel_rc.cpp
 */

namespace BatteryLevel {

static unsigned lastReading;

#ifndef VBATT_MAX
#define VBATT_MAX   0xfff
#endif

#ifndef VBATT_MIN
#define VBATT_MIN   0x888
#endif

void init() {
    lastReading = UNINITIALIZED;

    GPIOPin vbattMeas = VBATT_MEAS_GPIO;
    vbattMeas.setControl(GPIOPin::IN_ANALOG);

    VBATT_ADC.setCallback(VBATT_ADC_CHAN,BatteryLevel::adcCallback);
    VBATT_ADC.setSampleRate(VBATT_ADC_CHAN,Adc::SampleRate_239_5);

}

unsigned raw() {
    return lastReading;
}

unsigned vsys() {
    return _SYS_BATTERY_MAX;
}

unsigned scaled() {
    return MIN(lastReading, lastReading - VBATT_MIN) * (_SYS_BATTERY_MAX/(VBATT_MAX-VBATT_MIN));
}

void beginCapture() {
    VBATT_ADC.beginSample(VBATT_ADC_CHAN);
}

void adcCallback(uint16_t sample) {
#if 0
    //battery profiling
    static SysTime::Ticks timestamp = 0;
    if (SysTime::ticks() > SysTime::sTicks(timestamp+1)) {
        timestamp = SysTime::ticks()/SysTime::sTicks(1);
        UART("Battery level "); UART_HEX(timestamp); UART(" "); UART_HEX(sample); UART("\r\n");
    }
#endif
    lastReading = sample;

#ifndef DISABLE_VBATT_CHECK
    PowerManager::shutdownIfVBattIsCritical(lastReading, VBATT_MIN);
#endif
}

} // namespace BatteryLevel

#endif // USE_ADC_BATT_MEAS
