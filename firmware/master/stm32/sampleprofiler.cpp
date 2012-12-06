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
    timer.init(997, 70);
}

void SampleProfiler::onUSBData(const USBProtocolMsg &m)
{
    if (m.payloadLen() < 2 || m.payload[0] != SetProfilingEnabled)
        return;

    if (m.payload[1]) {
        timer.enableUpdateIsr();
        Tasks::trigger(Tasks::Profiler);
    } else {
        Tasks::cancel(Tasks::Profiler);
        timer.disableUpdateIsr();
    }
}

void SampleProfiler::processSample(uint32_t pc)
{
    timer.clearStatus();

    // high 4 bits are subsystem
    sampleBuf = (subsys << 28) | pc;
}

void SampleProfiler::task()
{
    USBProtocolMsg m(USBProtocol::Profiler);
    m.append((uint8_t*)&sampleBuf, sizeof sampleBuf);
    UsbDevice::write(m.bytes, m.len);
    Tasks::trigger(Tasks::Profiler);
}

void SampleProfiler::reportHang()
{
    /*
     * A hang was detected by the Tasks module. We're in the best
     * position to dump info about the state of the system at the time.
     */

    UART("HANG: Subsystem ");
    UART_HEX(subsys);
    UART("\r\n");

    /*
     * Stack dump is available for internal debugging, but disabled by
     * default as a security precaution.
     */

    #ifdef STACK_DUMP_ON_HANG
        volatile uint32_t sp;
        for (unsigned i = 0; i < 64; i++) {
            volatile uint32_t *p = &sp + i;
            UART("[");
            UART_HEX((uint32_t) p);
            UART("] ");
            UART_HEX(*p);
            UART("\r\n");
        }
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
