#ifndef SAMPLEPROFILER_H_
#define SAMPLEPROFILER_H_

#include "hwtimer.h"
#include "usbprotocol.h"

class SampleProfiler
{
public:

    enum SubSystem {
        None,
        AudioISR,
        AudioPull,
        SVCISR,
        RFISR,
    };

    enum Command {
        SetProfilingEnabled
    };

    static void init();

    static void onUSBData(const USBProtocolMsg &m);

    static void startSampling() {
        timer.enableUpdateIsr();
    }

    static void stopSampling() {
        timer.disableUpdateIsr();
    }

    static void processSample(uint32_t pc);
    static void task(void *p);

    static inline SubSystem subsystem() {
        return subsys;
    }

    static inline void setSubsystem(SubSystem s) {
        subsys = s;
    }

private:
    static SubSystem subsys;
    static uint32_t sampleBuf;  // currently just a single sample
    static HwTimer timer;
};

#endif // SAMPLEPROFILER_H_
