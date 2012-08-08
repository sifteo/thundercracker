#include "batterylevel.h"
#include "rctimer.h"
#include "board.h"

static uint16_t startTimestamp;
static int lastReading;

static const unsigned SYSTICK_PRESCALER = 25;
static unsigned systickPrescaler = 0;

namespace BatteryLevel {

void init()
{
    /*
     * BATT_MEAS can always remain configured as an input.
     * To charge (default state), BATT_MEAS_GND => input/float.
     * To discharge, BATT_MEAS_GND => driven low and timer capture on BATT_MEAS.
     */

    BATT_MEAS_GPIO.setControl(GPIOPin::IN_FLOAT);

    GPIOPin battMeasGnd = BATT_MEAS_GND_GPIO;
    battMeasGnd.setControl(GPIOPin::IN_FLOAT);
    battMeasGnd.setLow();

    /*
     * Note - this timer has already been configured in LED::init()
     * We just want to configure our channel.
     */
    HwTimer timer(&BATT_LVL_TIM);
    timer.disableChannel(BATT_LVL_CHAN);
    timer.configureChannelAsInput(BATT_LVL_CHAN, HwTimer::FallingEdge);
    timer.enableCompareCaptureIsr(BATT_LVL_CHAN);
    timer.enableChannel(BATT_LVL_CHAN);
}

int currentLevel()
{
    return lastReading;
}

void heartbeat()
{
    /*
     * The heartbeat ISR is our time base, but because our cap takes
     * up to 2.5s (!) to charge, divide the frequency appropriately
     * before kicking off a new sample.
     */

    if (++systickPrescaler == SYSTICK_PRESCALER) {
        systickPrescaler = 0;
        BATT_MEAS_GND_GPIO.setControl(GPIOPin::OUT_2MHZ);

        startTimestamp = HwTimer(&BATT_LVL_TIM).count();
    }
}

void captureIsr(uint16_t rawvalue)
{
    BATT_MEAS_GND_GPIO.setControl(GPIOPin::IN_FLOAT);

    /*
     * TBD what kind of treatment to give this signal
     */
    lastReading = uint16_t(rawvalue - startTimestamp);
}

} // namespace BatteryLevel


IRQ_HANDLER ISR_FN(BATT_LVL_TIM)()
{
    /*
     * NOTE! Battery level measurement and LED pwm are done on the same
     *       physical timer (ie, BATT_LVL_TIM == LED_PWM_TIM).
     *
     *       Because LWD pwms don't require any ISR attention, we implement
     *       the ISR for this timer here.
     */

    const HwTimer timer(&BATT_LVL_TIM);

    uint32_t status = timer.status();
    timer.clearStatus();

    if (status & (1 << BATT_LVL_CHAN)) {
        BatteryLevel::captureIsr(timer.lastCapture(BATT_LVL_CHAN));
    }
}
