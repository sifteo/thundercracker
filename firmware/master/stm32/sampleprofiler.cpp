#include "sampleprofiler.h"
#include "usb/usbdevice.h"
#include "usbprotocol.h"
#include "vectors.h"

#include "tasks.h"

#include <string.h>

SampleProfiler::SubSystem SampleProfiler::subsys;
uint32_t SampleProfiler::sampleBuf;
HwTimer SampleProfiler::timer(&PROFILER_TIM);

void SampleProfiler::init()
{
    subsys = None;

    // Highest prime number under 1000
    timer.init(997, 35);
}

void SampleProfiler::onUSBData(const USBProtocolMsg &m)
{
    if (m.payloadLen() < 2 || m.payload[0] != SetProfilingEnabled)
        return;

    if (m.payload[1])
        startSampling();
    else
        stopSampling();
}

void SampleProfiler::processSample(uint32_t pc)
{
    timer.clearStatus();

    // high 4 bits are subsystem
    sampleBuf = (subsys << 28) | pc;

    Tasks::setPending(Tasks::Profiler);
}

void SampleProfiler::task(void *p)
{
    Tasks::clearPending(Tasks::Profiler);

    USBProtocolMsg m(USBProtocol::Profiler);

    memcpy(m.payload, &sampleBuf, sizeof sampleBuf);
    m.len += sizeof(sampleBuf);
    UsbDevice::write(m.bytes, m.len);
}

/*
 * Take a peek at the stacked exception state to determine where we're
 * returning to.
 */
NAKED_HANDLER ISR_TIM2()
{
    asm volatile(
        "tst    lr, #0x4            \n\t"   // LR bit 2 determines if regs were stacked to user or main stack
        "ite    eq                  \n\t"   // load r1 with msp or psp based on that
        "mrseq  r1, msp             \n\t"
        "mrsne  r1, psp             \n\t"
        "ldr    r0, [r1, #24]       \n\t"   // load r0 with the stacked PC
        "push   { lr }              \n\t"
        "bl     %[handler]          \n\t"   // and pass it off to be processed
        "pop    { pc }"
        :
        : [handler] "i"(SampleProfiler::processSample)
    );
}
