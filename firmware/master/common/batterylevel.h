#ifndef BATTERYLEVEL_H
#define BATTERYLEVEL_H

#include <stdint.h>

namespace BatteryLevel
{
    void init();
    unsigned raw();
    unsigned vsys();
    unsigned scaled();
    void beginCapture();
    void captureIsr();
    void process(unsigned);
    void heartbeat();
    bool needWarning();
    void setWarningDone();
    void onCapture();
    void updatePercentage(int delta);
    unsigned getPercentage();
    static unsigned percentage = 100;
    static bool neverWarned = true;

    /*
     * Sentinel value to help determine whether a sample has successfully
     * been taken after startup.
     */
    static const unsigned UNINITIALIZED = 0xffff;

    /*
     * Empirically measured maximum jitter in battery level readings
     */
    static const unsigned MAX_JITTER = 0x81;
}

#endif // BATTERYLEVEL_H
