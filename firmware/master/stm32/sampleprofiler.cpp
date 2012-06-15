#include "sampleprofiler.h"
#include "usb/usbdevice.h"
#include "usbprotocol.h"
#include "vectors.h"

uint32_t SampleProfiler::sampleBuf;
HwTimer SampleProfiler::timer(&PROFILER_TIM);

void SampleProfiler::init()
{
    timer.init(1000, 50);
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

    sampleBuf = pc;

    // XXX: do usb transaction in a task? maybe buffer more samples so we're
    // less likely to overwrite our single buffered sample?

#if 0
    USBProtocolMsg m(USBProtocol::Profiler);

    memcpy(m.payload, &sampleBuf, sizeof sampleBuf);
    m.len += sizeof(sampleBuf);
    UsbDevice::write(m.bytes, m.len);
#endif
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
