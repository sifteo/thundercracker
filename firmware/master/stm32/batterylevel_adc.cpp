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

    /*
     * Delay required to charge up the internal reference cap.
     * It takes approximately 250 ms with R18 100k pullup.
     */

    while (SysTime::ticks() < SysTime::msTicks(300)) {
        ;
    }
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
    PowerManager::shutdownIfVBattIsCritical(lastReading, VBATT_MIN);
}

} // namespace BatteryLevel

#endif // USE_ADC_BATT_MEAS
