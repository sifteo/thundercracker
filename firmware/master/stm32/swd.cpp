/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "macros.h"
#include "swd.h"

#define SWD_RST_CYCLES  150
#define SWD_REQ_CYCLES  8
#define SWD_ACK_CYCLES  3
#define SWD_DAT_CYCLES  32

#define SWD_ACK_OK      1
#define SWD_ACK_WAIT    2
#define SWD_ACK_ERR     4
#define SWD_ACK_DISC    7

#define SWD_ID_REQ      0xa5

#define DEBUG_SWD 1

void SWDMaster::init()
{
    swdclk.setLow();
    swdclk.setControl(GPIOPin::OUT_2MHZ);
    swdio.setHigh();
    swdio.setControl(GPIOPin::OUT_2MHZ);
}

void SWDMaster::startTimer()
{
    busyFlag = true;
    swdTimer.init(0x7fff,0x0);  //Tclk = 2*count/72 us
    swdTimer.enableUpdateIsr();
}

void SWDMaster::stopTimer()
{
    swdTimer.deinit();
    busyFlag = false;
}

/*
 *  Leaves finally in the following state
 *  SWDCLK  = low
 *  SWDIO   = unchanged (but stays high throughout)
 *  timer   = stopped
 */
void SWDMaster::wkpControllerCB()
{
    static uint32_t cycles = SWD_RST_CYCLES;

    if (isRisingEdge()) {
#ifdef DEBUG_SWD
        if (swdio.isHigh()) {
            UART("1 ");
        } else {
            UART("0 ");
        }
#endif
        return;
    }

    fallingEdge();
    if(--cycles == 0) {
        cycles = SWD_RST_CYCLES;
        //send an ID req immediately
        header = SWD_ID_REQ;
        controllerCB = &SWDMaster::txnControllerCB;
    }
}

/*
 *  Leaves finally in the following state
 *  SWDCLK  = low
 *  SWDIO   = high
 *  timer   = stopped
 */
void SWDMaster::txnControllerCB()
{
    enum swdphase {
        _idle,
        _header,
        _turn1,
        _ack,
        _turn2,
        _data,
        _parity,
        _turn3,
        _reset,
        _done
    };
    static uint32_t buffer = 0;
    static swdphase state = _idle;
    static uint8_t cycles = SWD_REQ_CYCLES;
#ifdef DEBUG_SWD
    static bool rnw = false;
#endif

    if(isRisingEdge()) {
#ifdef DEBUG_SWD
        if (!rnw) {
            if (swdio.isHigh()) {
                UART("1w ");
            } else {
                UART("0w ");
            }
        }
#endif
        return;
    }

    fallingEdge();
#ifdef DEBUG_SWD
    if (rnw) {
        if (swdio.isHigh()) {
            UART("1r ");
        } else {
            UART("0r ");
        }
    }
#endif

    switch(state) {
    case _idle:
        // fall through to header state;
        // start bit has been Xmitted before we enter FSM
        buffer = header<<(24+1);
        cycles = SWD_REQ_CYCLES-1;
        state = _header;
    case _header:
        sendbit(buffer);
        if (--cycles == 0) {
            //buffer should be clear here
            state = _turn1;
        }
        break;
    case _turn1:
        swdio.setLow();
        cycles = SWD_ACK_CYCLES;
        state = _ack;
        swdio.setControl(GPIOPin::IN_FLOAT);
#ifdef DEBUG_SWD
        rnw = true;
#endif
        break;
    case _ack:
        receivebit(buffer);
        if (--cycles == 0) {
            state = _turn2;
        }
        break;
    case _turn2:
#if 1
        // DEBUG_ACK
        swdio.setHigh();
        swdio.setControl(GPIOPin::OUT_2MHZ);
        stopTimer();
#ifdef DEBUG_SWD
        rnw = false;
        UART("\r\n");
#endif
        state = _idle;
        break;
#else
        if (buffer == SWD_ACK_OK) {
            cycles = SWD_DAT_CYCLES;
            state = _data;
            if (!isDAPRead()) {
                swdio.setControl(GPIOPin::OUT_2MHZ);
#ifdef DEBUG_SWD
                rnw = false;
#endif
                buffer = payloadOut;
            } else {
                buffer = 0;
            }
        } else {
            // TODO handle wait/error case differently
            swdio.setHigh();
            swdio.setControl(GPIOPin::OUT_2MHZ);
            cycles = SWD_RST_CYCLES-1;
            state = _reset;
            break;
        }
        //fall through to data for DAP read
#endif
    case _data:
        if (isDAPRead()) {
            receivebit(buffer);
        } else {
            sendbit(buffer);
        }
        if (--cycles == 0) {
            state = _parity;
        }
        break;
    case _parity:
        // TODO implement parity check
        if (isDAPRead()) {
            //read and compare parity
            //readbit(buffer)
        } else {
            //compute and write parity
            //sendbit(buffer);
        }
        state = _turn3;
        break;
    case _turn3:
        swdio.setHigh();
        swdio.setControl(GPIOPin::OUT_2MHZ);
#ifdef DEBUG_SWD
        rnw = false;
#endif
        state = _done;
        break;
    case _done:
#ifdef DEBUG_SWD
        UART("\r\n");
#endif
        stopTimer();
        state = _idle;
        break;
    case _reset:
        if (--cycles == 0) {
            //buffer should be clear here
            //state = _idle;  //retry
            state = _done;  //giveup
        }
        break;
    default:
        //shouldnt come here
        //TODO assert
        break;
    }
}

bool SWDMaster::enableRequest()
{
    if(busyFlag)
        return false;

    controllerCB = &SWDMaster::wkpControllerCB;
    startTimer();
    return true;
}

bool SWDMaster::readRequest(uint8_t hdr)
{
    if(busyFlag)
        return false;

    header = hdr;
    controllerCB = &SWDMaster::txnControllerCB;
    startTimer();
    return true;
}

bool SWDMaster::writeRequest(uint8_t hdr, uint32_t payload)
{
    if(busyFlag)
        return false;

    header = hdr; payloadOut = payload;
    controllerCB = &SWDMaster::txnControllerCB;
    startTimer();
    return true;
}

void SWDMaster::isr()
{
    // Acknowledge IRQ by clearing timer status
    swdTimer.clearStatus();
    (this->*controllerCB)();
}
