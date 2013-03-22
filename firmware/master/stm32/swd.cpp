/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "swd.h"
#include "usb/usbdevice.h"  //DEBUG

#define SWD_WKP_CYCLES 2*5
#define SWD_REQ_CYCLES 2*5
#define SWD_ACK_CYCLES 2*3
#define SWD_DAT_CYCLES 2*32

void ALWAYS_INLINE SWDMaster::clk_toggle()
{
    if(swdclk.isLow()) {
        swdclk.setHigh();
    } else {
        swdclk.setLow();
    }
}

void SWDMaster::init()
{
    swdTimer.init(0x7fff,0x0);
    swdclk.setControl(GPIOPin::OUT_2MHZ);
    swdclk.setLow();
    swdio.setControl(GPIOPin::OUT_2MHZ);
    swdio.setHigh();
}

void SWDMaster::start()
{
    state = wkp;
    swdTimer.enableUpdateIsr();
}

void SWDMaster::stop()
{
    swdTimer.disableUpdateIsr();
}

void SWDMaster::controller()
{
    static unsigned pendingCycles = SWD_WKP_CYCLES;
    static swdState microstate = state;

#if 0

    // DEBUG code
    clk_toggle();
    return;

#else

    if (microstate == rtz) {
        swdclk.setLow();
        microstate = state;
    } else {
        switch (microstate) {
        case idle:
            swdclk.setHigh();
            microstate = rtz;
            break;
        case wkp:
            if (pendingCycles--) {
                swdclk.setHigh();
                microstate = rtz;
            } else {
                pendingCycles = SWD_REQ_CYCLES;
                state = req;
                microstate = rtz;
            }
            break;
        case req:
            if (pendingCycles--) {
                microstate = rtz;
            } else {
                pendingCycles = SWD_WKP_CYCLES;
                state = wkp;
                microstate = rtz;
            }
            break;
        default:
            break;
        }
    }
    //const uint8_t response[] = { 255, microstate, state};
    //UsbDevice::write(response, sizeof response);
    //return;

#endif

}

void SWDMaster::isr()
{
    // Acknowledge IRQ by clearing timer status
    swdTimer.clearStatus();

    controller();
}
