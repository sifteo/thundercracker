#include "batterylevel.h"
#include "rctimer.h"
#include "board.h"
#include "neighbor_tx.h"
#include "neighbor_protocol.h"
#include "powermanager.h"

static int lastReading;

/*
 * As we're sharing a timer with NeighborTX, the end of each neighbor transmission
 * is an opportunity for us to begin a sample. However, our charge time is max
 * 2.5s, so we never want to run more frequently than that.
 *
 * 2500 / (NUM_TX_WAIT_PERIODS * BIT_PERIOD_TICKS * TICK) ~= 833
 *
 * See neighbor_protocol.h for neighbor transmission periods.
 */
static unsigned chargePrescaleCounter = 0;
static const unsigned CHARGE_PRESCALER = 833;

/*
 * Max discharge time is 50ms.
 * To configure our timer at this resolution, we need to use the maximum
 * period (0xffff), and select a prescaler that gets us as close as possible.
 *
 * 0.05 / ((1 / 36000000) * 0xffff) == 27.466
 */
static const unsigned DISCHARGE_PRESCALER = 3;

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
     * We share this timer with NeighborTX - its alrady been init'd there.
     * Just configure our channel, but don't enable it's ISR yet.
     */

    HwTimer timer(&BATT_LVL_TIM);
    timer.configureChannelAsInput(BATT_LVL_CHAN, HwTimer::FallingEdge);
    timer.enableChannel(BATT_LVL_CHAN);
}

int currentLevel()
{
    return lastReading;
}

void beginCapture()
{
    /*
     * An opportunity to take a new sample.
     *
     * First, detect whether a battery is connected: BATT_MEAS will be high.
     *
     * BATT_LVL_TIM is shared with neighbor transmit
     */

    if (chargePrescaleCounter++ == CHARGE_PRESCALER) {

        chargePrescaleCounter = 0;

        if (BATT_MEAS_GPIO.isLow())
            return;

        NeighborTX::pause();

        HwTimer timer(&BATT_LVL_TIM);
        timer.setPeriod(0xffff, DISCHARGE_PRESCALER);
        timer.setCount(0);

        BATT_MEAS_GND_GPIO.setControl(GPIOPin::OUT_2MHZ);
        BATT_MEAS_GND_GPIO.setLow();

        timer.enableCompareCaptureIsr(BATT_LVL_CHAN);
    }
}

void captureIsr()
{
    /*
     * Called from within NeighborTX, where the interrupts for the
     * timer we're on are handled.
     *
     * Once our discharge has completed, we can resume normal neighbor transmission.
     */
    BATT_MEAS_GND_GPIO.setControl(GPIOPin::IN_FLOAT);

    HwTimer timer(&BATT_LVL_TIM);
    timer.disableCompareCaptureIsr(BATT_LVL_CHAN);

    /*
     * We don't need to track a start time, as we always reset the timer
     * before kicking off a new capture.
     *
     * TBD what kind of treatment to give this signal.
     */
    lastReading = timer.lastCapture(BATT_LVL_CHAN);

    NeighborTX::resume();
}

} // namespace BatteryLevel
